import numpy as np
import time

def run_benchmark():
    num_vectors = 1_000_000
    dim = 128
    num_queries = 10000 

    print("GENERATING SYNTHETIC DATA...")
    print(f"Database: {num_vectors} vectors, {dim} dimensions")
    
    db = np.random.random((num_vectors, dim)).astype(np.float32)
    queries = np.random.random((num_queries, dim)).astype(np.float32)

    print("WARMING UP CPU/BLAS...")
    for i in range(10):
        _ = np.dot(db, queries[i])

    print("STARTING SEARCH QUERIES...")
    latencies = []
    
    for i in range(num_queries):
        q = queries[i]
        
        t1 = time.perf_counter()

        scores = np.dot(db, q)

        top_k_idx = np.argpartition(scores, -100)[-100:]
        
        t2 = time.perf_counter()
        
        latencies.append((t2 - t1) * 1_000_000) 

    latencies = np.sort(latencies)
    total_time_ms = np.sum(latencies) / 1000.0
    throughput = num_queries / (total_time_ms / 1000.0)
    
    print("\n===========================================")
    print("       NUMPY BRUTE-FORCE BENCHMARK         ")
    print("===========================================")
    print(f"VECTORS: {num_vectors} | DIMENSION: {dim} | QUERIES: {num_queries}")
    print(f"TOTAL SEARCH TIME: {total_time_ms:.2f} ms")
    print(f"SEARCH THROUGHPUT: {throughput:.2f} queries/sec")
    print("-------------------------------------------")
    print("LATENCY PERCENTILES (Microseconds):")
    print(f"  Min:     {latencies[0]:.2f} us")
    print(f"  Avg:     {np.mean(latencies):.2f} us")
    print(f"  Median:  {np.median(latencies):.2f} us")
    print(f"  p90:     {np.percentile(latencies, 90):.2f} us")
    print(f"  p95:     {np.percentile(latencies, 95):.2f} us")
    print(f"  p99:     {np.percentile(latencies, 99):.2f} us")
    print(f"  p99.9:   {np.percentile(latencies, 99.9):.2f} us")
    print(f"  Max:     {latencies[-1]:.2f} us")
    print("===========================================")

if __name__ == "__main__":
    run_benchmark()