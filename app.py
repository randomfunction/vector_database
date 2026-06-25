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
        print("snapshot.bin not found. Building database from cc.en.300.vec...")
        db.reserve(2000000)
        with open("cc.en.300.vec","r",encoding="utf-8") as f:
            next(f) # Skip header
            idx=0
            for line in f:
                parts=line.split(' ',1)
                if len(parts)>0:
                    word=parts[0]
                    # Dynamically generate the 300-dim vector to guarantee no OOV issues
                    vec=ft_model.get_word_vector(word).tolist()
                    db.insert(str(idx),vec,word)
                    idx+=1
                    if idx%100000==0:
                        print(f"Inserted {idx} words...")
        print("Saving snapshot.bin to disk...")
        db.save_to_file("snapshot.bin")
    
    print("Database ready!")
    yield

app=FastAPI(lifespan=lifespan)

@app.get("/search")
def search(word:str,k:int=10):
    vec=ft_model.get_word_vector(word).tolist()
    results=db.search(vec,k)
    return [{"word":res.metadata,"distance":res.distance} for res in results]
