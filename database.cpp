#include"database.hpp"

Engine::Engine(MetricType m){
    metric=m;
}

void Engine::set_metric(MetricType m){
    unique_lock<shared_mutex> lock(rw_lock);
    metric=m;
}

void Engine::insert(const string&uuid,const vector<float>&emb,const string&data){
    unique_lock<shared_mutex> lock(rw_lock);
    if(uuid_to_id.count(uuid)){
        return;
    }
    int newid=vectors.size();
    uuid_to_id[uuid]=newid;
    id_to_uuid.push_back(uuid);
    vectors.push_back(emb);
    metadata.push_back(data);
}

void Engine::update(const string&uuid,const vector<float>&emb,const string&data){
    unique_lock<shared_mutex> lock(rw_lock);
    if(!uuid_to_id.count(uuid)){
        return;
    }
    int id=uuid_to_id[uuid];
    vectors[id]=emb;
    metadata[id]=data;
}

void Engine::remove(const string&uuid){
    unique_lock<shared_mutex> lock(rw_lock);
    if(!uuid_to_id.count(uuid)){
        return;
    }
    int id=uuid_to_id[uuid];
    int last_id=vectors.size()-1;
    if(id!=last_id){
        string last_uuid=id_to_uuid[last_id];
        vectors[id]=move(vectors[last_id]);
        metadata[id]=move(metadata[last_id]);
        id_to_uuid[id]=move(last_uuid);
        uuid_to_id[last_uuid]=id;
    }
    vectors.pop_back();
    metadata.pop_back();
    id_to_uuid.pop_back();
    uuid_to_id.erase(uuid);
}

float Engine::squared_eucledian_distance(const vector<float>&a,const vector<float>&b) const{
    float res=0;
    int n=a.size();
    for(int i=0;i<n;i++){
        res+=(a[i]-b[i])*(a[i]-b[i]);
    }
    return res;
}

float Engine::dot_product(const vector<float>&a,const vector<float>&b) const{
    float ans=0;
    int n=a.size();
    for(int i=0;i<n;i++){
        ans+=a[i]*b[i];
    }
    return ans;
}

float Engine::cosine_similarity(const vector<float>&a,const vector<float>&b) const{
    float dot=0,normA=0,normB=0;
    int n=a.size();
    for(int i=0;i<n;i++){
        dot+=a[i]*b[i];
        normA+=a[i]*a[i];
        normB+=b[i]*b[i];
    }
    if(normA==0||normB==0){
        return 0;
    }
    return dot/(sqrt(normA)*sqrt(normB));
}

float Engine::compute_distance(const vector<float>&a,const vector<float>&b) const{
    if(metric==MetricType::L2){
        return squared_eucledian_distance(a,b);
    }
    if(metric==MetricType::DOT){
        return -dot_product(a,b);
    }
    if(metric==MetricType::COSINE){
        return -cosine_similarity(a,b);
    }
    return 0;
}

vector<SearchResult> Engine::search(const vector<float>&query,int k){
    shared_lock<shared_mutex> lock(rw_lock);
    if(vectors.size()==0){
        return {};
    }
    priority_queue<pair<float,int>> pq;
    for(int i=0;i<vectors.size();i++){
        float dist=compute_distance(vectors[i],query);
        pq.push({dist,i});
        if(pq.size()>k){
            pq.pop();
        }
    }
    vector<SearchResult> res;
    while(!pq.empty()){
        auto [dist,i]=pq.top();
        pq.pop();
        float real_dist=(metric==MetricType::L2)?dist:-dist;
        res.push_back({id_to_uuid[i],real_dist,metadata[i]});
    }
    reverse(res.begin(),res.end());
    return res;
}

void Engine::save_to_file(const string&filename){
    shared_lock<shared_mutex> lock(rw_lock);
    ofstream file(filename,ios::out|ios::binary);
    if(!file.is_open()){
        return;
    }
    int m_type=static_cast<int>(metric);
    file.write(reinterpret_cast<const char*>(&m_type),sizeof(m_type));
    int n=vectors.size();
    file.write(reinterpret_cast<const char*>(&n),sizeof(n));
    if(n==0){
        return;
    }
    int dimensions=vectors[0].size();
    file.write(reinterpret_cast<const char*>(&dimensions),sizeof(dimensions));
    for(int i=0;i<n;i++){
        int uuid_len=id_to_uuid[i].size();
        file.write(reinterpret_cast<const char*>(&uuid_len),sizeof(uuid_len));
        file.write(id_to_uuid[i].data(),uuid_len);
        file.write(reinterpret_cast<const char*>(vectors[i].data()),dimensions*sizeof(float));
        int meta_len=metadata[i].size();
        file.write(reinterpret_cast<const char*>(&meta_len),sizeof(meta_len));
        file.write(metadata[i].data(),meta_len);
    }
    file.close();
}

void Engine::load_from_file(const string&filename){
    unique_lock<shared_mutex> lock(rw_lock);
    ifstream file(filename,ios::in|ios::binary);
    if(!file.is_open()){
        return;
    }
    vectors.clear();
    metadata.clear();
    uuid_to_id.clear();
    id_to_uuid.clear();
    int m_type=0;
    file.read(reinterpret_cast<char*>(&m_type),sizeof(m_type));
    metric=static_cast<MetricType>(m_type);
    int n=0;
    file.read(reinterpret_cast<char*>(&n),sizeof(n));
    if(n==0){
        file.close();
        return;
    }
    int dimensions=0;
    file.read(reinterpret_cast<char*>(&dimensions),sizeof(dimensions));
    for(int i=0;i<n;i++){
        int uuid_len=0;
        file.read(reinterpret_cast<char*>(&uuid_len),sizeof(uuid_len));
        string uuid(uuid_len,'\0');
        file.read(&uuid[0],uuid_len);
        vector<float> vec(dimensions);
        file.read(reinterpret_cast<char*>(vec.data()),dimensions*sizeof(float));
        int meta_len=0;
        file.read(reinterpret_cast<char*>(&meta_len),sizeof(meta_len));
        string meta(meta_len,'\0');
        file.read(&meta[0],meta_len);
        int newid=vectors.size();
        uuid_to_id[uuid]=newid;
        id_to_uuid.push_back(uuid);
        vectors.push_back(vec);
        metadata.push_back(meta);
    }
    file.close();
}