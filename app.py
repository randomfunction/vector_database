from fastapi import FastAPI
from contextlib import asynccontextmanager
import fasttext
import custom_vectordb
import os

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
            if idx>=50000:
                break
        print("Saving snapshot.bin to disk...")
        db.save_to_file("snapshot.bin")
    
    print("Database ready!")
    yield

from fastapi.middleware.cors import CORSMiddleware

app=FastAPI(lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/search")
def search(word:str,k:int=11):
    vec=ft_model.get_word_vector(word).tolist()
    results=db.search(vec,k)
    results.pop(0)
    return {
        "query_vector": vec,
        "results": [
            {
                "word": res.metadata,
                "distance": res.distance,
                "vector": ft_model.get_word_vector(res.metadata).tolist()
            } for res in results
        ]
    }
