# C++ Vector Database Engine (Conceptual)

This project is an educational, from-scratch implementation of a Vector Database in C++17. 

**Disclaimer:** This is *not* intended to be a production-ready database (like Milvus, Qdrant, or Pinecone). Instead, it was built with a focus on **conceptual clarity** to thoroughly understand the mathematical and systemic mechanics of how a vector engine works under the hood. 

By avoiding heavy third-party abstractions, this project implements core database and network layers entirely from scratch, demonstrating a deep understanding of systems engineering, memory management, and linear algebra.

## Core Concepts Implemented

* **In-Memory Storage & CRUD:** Direct management of high-dimensional vectors, metadata strings, and UUID mappings. Features robust `insert`, `update`, and `remove` operations (using $O(1)$ swap-and-pop techniques).
* **Similarity Search (Exact KNN):** Implements an exhaustive "Flat" search approach utilizing a Max-Heap (`std::priority_queue`) to efficiently track and return the K-Nearest Neighbors.
* **Mathematical Distance Metrics:** Supports runtime-switchable metrics to measure vector similarity:
  * L2 (Squared Euclidean Distance)
  * Dot Product
  * Cosine Similarity
* **Concurrency & Thread Safety:** Utilizes `std::shared_mutex` to implement Read-Write locks, allowing multiple concurrent read queries while safely locking during write/update operations.
* **Data Persistence:** A custom binary serialization/deserialization engine that efficiently packs and unpacks the exact memory layout of the database to a binary file (`db.bin`) for durable state storage.
* **Zero-Dependency REST API:** Includes a standalone, custom-built TCP Socket Server (`server.cpp`) that speaks HTTP and parses JSON payloads to expose the database capabilities over a network without relying on external HTTP libraries.

## What is Missing (The Production Path)
To transition this from a conceptual learning engine to a production cluster, it would require:
1. **Approximate Nearest Neighbors (ANN):** Implementing HNSW or IVF indexes to avoid $O(N)$ exhaustive search times on massive datasets.
2. **Memory-Mapped Files (mmap):** To allow dataset sizes larger than available RAM.
3. **Distributed Consensus:** Integrating protocols like Raft for multi-node clustering, sharding, and replication.

## Getting Started

### 1. Running the Database Tests
A comprehensive test suite is available in `main.cpp` that verifies mathematical accuracy, CRUD operations, persistence, and thread-safety basics.
```bash
g++ -std=c++17 main.cpp database.cpp -o test_db
./test_db
```

### 2. Running the REST API Server
You can launch the custom TCP/HTTP server to interact with the database via network requests.
```bash
g++ -std=c++17 server.cpp database.cpp -o server
./server
```
*(The server binds to port 8080 and handles HTTP POST endpoints like `/insert`, `/search`, `/update`, and `/delete`.)*
