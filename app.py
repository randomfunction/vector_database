from fastapi import FastAPI, Query, HTTPException
from contextlib import asynccontextmanager
import fasttext
import custom_vectordb
import os
import Levenshtein

ft_model=None
db=None

@asynccontextmanager
async def lifespan(app:FastAPI):
    global ft_model
    global db
    
    print("Loading FastText model...")
    ft_model=fasttext.load_model("cc.en.300.bin")
    
    # Initialize Engine with strict 300 dimensions implicitly assumed in code structure, mapped to COSINE
    db=custom_vectordb.Engine(custom_vectordb.MetricType.COSINE)
    
    if os.path.exists("snapshot.bin"):
        print("Found snapshot.bin! Loading database from disk...")
        db.load_from_file("snapshot.bin")
    else:
        print("snapshot.bin not found. Building database directly from FastText binary model...")
        words = ft_model.get_words()
        db.reserve(len(words) + 1000)
        idx=0
        for word in words:
            vec=ft_model.get_word_vector(word).tolist()
            db.insert(str(idx),vec,word)
            idx+=1
            if idx%1000==0:
                print(f"Inserted {idx} words...")
            if idx>=500000:
                break
        print("Saving snapshot.bin to disk...")
        db.save_to_file("snapshot.bin")
    
    print("Database ready!")
    yield

from fastapi.middleware.cors import CORSMiddleware

app=FastAPI(lifespan=lifespan)

# Pin allowed origins instead of "*". The API uses no cookies/auth, so
# credentials are disabled (the "*" + credentials combination is invalid and
# insecure). Override the list per environment via ALLOWED_ORIGINS
# (comma-separated); defaults cover local Vite/React dev servers.
_origins_env=os.environ.get("ALLOWED_ORIGINS","http://localhost:5173,http://localhost:3000")
ALLOWED_ORIGINS=[o.strip() for o in _origins_env.split(",") if o.strip()]

app.add_middleware(
    CORSMiddleware,
    allow_origins=ALLOWED_ORIGINS,
    allow_credentials=False,
    allow_methods=["GET"],
    allow_headers=["*"],
    max_age=86400,
)

ALLOWED_METRICS={"cosine","l2","dot"}


def sim(v1,v2):
    return Levenshtein.ratio(v1,v2)

@app.get("/search")
def search(
    word:str=Query(...,min_length=1,max_length=100),
    k:int=Query(...,ge=1,le=100),
    metric:str=Query("cosine"),
):
    word=word.strip()
    if not word:
        raise HTTPException(status_code=400,detail="word must not be empty")
    if metric.lower() not in ALLOWED_METRICS:
        raise HTTPException(status_code=400,detail=f"metric must be one of {sorted(ALLOWED_METRICS)}")

    search_k= k+10
    vec=ft_model.get_word_vector(word).tolist()

    results=db.search(vec,search_k)

    count=0;

    final_results=[]
    for i in results:

        if(sim(i.metadata,word)<0.7):
            final_results.append(i)
            count+=1

        if(count==k):
            break

    results=final_results

    for i in final_results:
        print(i.metadata,sim(i.metadata,word))

    return {
        "query_vector": [round(v, 3) for v in vec],
        "results": [
            {
                "word": res.metadata,
                "distance": round(float(res.distance), 4),
                "vector": [round(v, 3) for v in ft_model.get_word_vector(res.metadata).tolist()]
    
            } for res in results
        ]
    }
