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

class Engine{
    unordered_map<string,int> uuid_to_id;
    vector<string> id_to_uuid;
    vector<vector<float>> vectors;
    vector<string> metadata;
    MetricType metric;
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
    float compute_distance(const vector<float>&a,const vector<float>&b) const;
    float squared_eucledian_distance(const vector<float>&a,const vector<float>&b) const;
    float dot_product(const vector<float>&a,const vector<float>&b) const;
    float cosine_similarity(const vector<float>&a,const vector<float>&b) const;
};
