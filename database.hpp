#pragma once
#include<bits/stdc++.h>
#include<shared_mutex>
using namespace std;

enum class MetricType{
    L2,
    DOT,
    COSINE
};

struct SearchResult{
    string uuid;
    float dist;
    string metadata;
};

template<class T>
struct AlignedAllocator{
    using value_type=T;
    AlignedAllocator()noexcept{}
    template<class U>AlignedAllocator(const AlignedAllocator<U>&)noexcept{}
    T* allocate(size_t n){
        size_t alignment=64;
        size_t bytes=n*sizeof(T);
        size_t remainder=bytes%alignment;
        if(remainder!=0){
            bytes+=alignment-remainder;
        }
        void* p=aligned_alloc(alignment,bytes);
        if(!p){
            throw bad_alloc();
        }
        return static_cast<T*>(p);
    }
    void deallocate(T* p,size_t)noexcept{
        free(p);
    }
    template<class U>bool operator==(const AlignedAllocator<U>&)const noexcept{return true;}
    template<class U>bool operator!=(const AlignedAllocator<U>&)const noexcept{return false;}
};

class Engine{
    unordered_map<string,int> uuid_to_id;
    vector<string> id_to_uuid;
    vector<float,AlignedAllocator<float>> flat_vectors;
    vector<string> metadata;
    MetricType metric;
    int dim;
    mutable shared_mutex rw_lock;

    public:
    Engine(MetricType m=MetricType::L2);
    void insert(const string&uuid,const vector<float>&emb,const string&data);
    void update(const string&uuid,const vector<float>&emb,const string&data);
    void remove(const string&uuid);
    vector<SearchResult> search(const vector<float>&query,int k);
    void save_to_file(const string&filename);
    void load_from_file(const string&filename);
    void set_metric(MetricType m);

    private:
    float compute_distance(const float*a,const vector<float>&b) const;
    float squared_eucledian_distance(const float*a,const vector<float>&b) const;
    float dot_product(const float*a,const vector<float>&b) const;
    float cosine_similarity(const float*a,const vector<float>&b) const;
};
