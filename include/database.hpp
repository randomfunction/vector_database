#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <shared_mutex>
#include <cstddef>
#include <cstdlib>
#include <new>

// Portability: only pull in x86 SIMD intrinsics on x86 targets. On every other
// architecture (e.g. Apple Silicon / ARM) the engine falls back to scalar
// kernels, and the chosen kernel is decided at runtime, not at compile time.
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  #define VDB_X86 1
  #include <immintrin.h>
#else
  #define VDB_X86 0
#endif

enum class MetricType{
    L2,
    DOT,
    COSINE
};

struct SearchResult{
    std::string uuid;
    float dist;
    std::string metadata;
};

struct HNSWNode{
    int id;
    int max_layer;
    int edge_offset;
};

template<class T>
struct AlignedAllocator{
    using value_type=T;
    AlignedAllocator()noexcept{}
    template<class U>AlignedAllocator(const AlignedAllocator<U>&)noexcept{}
    T* allocate(std::size_t n){
        std::size_t alignment=64;
        std::size_t bytes=n*sizeof(T);
        std::size_t remainder=bytes%alignment;
        if(remainder!=0){
            bytes+=alignment-remainder;
        }
        void* p=aligned_alloc(alignment,bytes);
        if(!p){
            throw std::bad_alloc();
        }
        return static_cast<T*>(p);
    }
    void deallocate(T* p,std::size_t)noexcept{
        free(p);
    }
    template<class U>bool operator==(const AlignedAllocator<U>&)const noexcept{return true;}
    template<class U>bool operator!=(const AlignedAllocator<U>&)const noexcept{return false;}
};

class Engine{
    std::unordered_map<std::string,int> uuid_to_id;
    std::vector<std::string> id_to_uuid;
    std::vector<float,AlignedAllocator<float>> flat_vectors;
    std::vector<std::string> metadata;
    std::vector<bool> active;
    std::vector<HNSWNode> nodes;
    std::vector<int,AlignedAllocator<int>> flat_edges;
    MetricType metric;
    int dim;
    mutable std::shared_mutex rw_lock;
    bool has_simd;   // AVX2+FMA available on this CPU (decided once, at construction)

    int max_layer=-1;
    double mult;
    int ep_id=-1;

    int M = 32;               // Denser connections
    int M_max_0 = 64;         // Denser base layer
    int ef_construction = 400; // High-quality graph build
    int ef_search = 200;      // Wide search beam
    std::size_t max_capacity = 1000000;

    public:
    Engine(MetricType m=MetricType::L2);
    void reserve(std::size_t max_elements);
    void insert(const std::string&uuid,const std::vector<float>&emb,const std::string&data);
    void update(const std::string&uuid,const std::vector<float>&emb,const std::string&data);
    void remove(const std::string&uuid);
    std::vector<SearchResult> search(const std::vector<float>&query,int k);
    void save_to_file(const std::string&filename);
    void load_from_file(const std::string&filename);
    void set_metric(MetricType m);

    private:
    static bool detect_simd();
    float compute_distance(const float*a,const float*b) const;

    // Scalar distance kernels: always compiled, run on any CPU.
    float l2_scalar(const float*a,const float*b) const;
    float dot_scalar(const float*a,const float*b) const;
    float cosine_scalar(const float*a,const float*b) const;

#if VDB_X86
    // AVX2+FMA distance kernels: compiled on x86, selected at runtime via has_simd.
    static float sum256(__m256 x);
    float l2_avx2(const float*a,const float*b) const;
    float dot_avx2(const float*a,const float*b) const;
    float cosine_avx2(const float*a,const float*b) const;
#endif

    int get_edge_offset(int node_offset,int layer) const;
    int* get_edges(int node_id,int layer);
    const int* get_edges(int node_id,int layer) const;
    void set_edges(int node_id,int layer,const std::vector<int>&edges);
    void search_layer(const float*query,int entry_point_id,int ef,int layer,std::vector<std::pair<float,int>>&top_candidates) const;
    void prune_connections(int node_id,int layer,int max_conn);
};
