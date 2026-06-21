#include"database.hpp"

Engine::Engine(MetricType m){
    metric=m;
    dim=0;
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
    if(dim==0){
        dim=emb.size();
    }
    else if(emb.size()!=dim){
        return;
    }
    int newid=id_to_uuid.size();
    uuid_to_id[uuid]=newid;
    id_to_uuid.push_back(uuid);
    flat_vectors.insert(flat_vectors.end(),emb.begin(),emb.end());
    metadata.push_back(data);
}

void Engine::update(const string&uuid,const vector<float>&emb,const string&data){
    unique_lock<shared_mutex> lock(rw_lock);
    if(!uuid_to_id.count(uuid)){
        return;
    }
    if(emb.size()!=dim){
        return;
    }
    int id=uuid_to_id[uuid];
    for(int i=0;i<dim;i++){
        flat_vectors[id*dim+i]=emb[i];
    }
    metadata[id]=data;
}

void Engine::remove(const string&uuid){
    unique_lock<shared_mutex> lock(rw_lock);
    if(!uuid_to_id.count(uuid)){
        return;
    }
    int id=uuid_to_id[uuid];
    int last_id=id_to_uuid.size()-1;
    if(id!=last_id){
        string last_uuid=id_to_uuid[last_id];
        for(int i=0;i<dim;i++){
            flat_vectors[id*dim+i]=flat_vectors[last_id*dim+i];
        }
        metadata[id]=move(metadata[last_id]);
        id_to_uuid[id]=move(last_uuid);
        uuid_to_id[last_uuid]=id;
    }
    flat_vectors.resize(last_id*dim);
    metadata.pop_back();
    id_to_uuid.pop_back();
    uuid_to_id.erase(uuid);
    if(id_to_uuid.size()==0){
        dim=0;
    }
}

float Engine::sum256(__m256 x) const{
    float arr[8];
    _mm256_storeu_ps(arr,x);
    return arr[0]+arr[1]+arr[2]+arr[3]+arr[4]+arr[5]+arr[6]+arr[7];
}

float Engine::squared_eucledian_distance(const float*a,const vector<float>&b) const{
    float res=0;
    int i=0;
    __m256 sum=_mm256_setzero_ps();
    const float*b_ptr=b.data();
    for(;i<=dim-8;i+=8){
        __m256 va=_mm256_load_ps(a+i);
        __m256 vb=_mm256_loadu_ps(b_ptr+i);
        __m256 diff=_mm256_sub_ps(va,vb);
        sum=_mm256_fmadd_ps(diff,diff,sum);
    }
    res=sum256(sum);
    for(;i<dim;i++){
        res+=(a[i]-b_ptr[i])*(a[i]-b_ptr[i]);
    }
    return res;
}

float Engine::dot_product(const float*a,const vector<float>&b) const{
    float ans=0;
    int i=0;
    __m256 sum=_mm256_setzero_ps();
    const float*b_ptr=b.data();
    for(;i<=dim-8;i+=8){
        __m256 va=_mm256_load_ps(a+i);
        __m256 vb=_mm256_loadu_ps(b_ptr+i);
        sum=_mm256_fmadd_ps(va,vb,sum);
    }
    ans=sum256(sum);
    for(;i<dim;i++){
        ans+=a[i]*b_ptr[i];
    }
    return ans;
}

float Engine::cosine_similarity(const float*a,const vector<float>&b) const{
    float dot=0,normA=0,normB=0;
    int i=0;
    __m256 s_dot=_mm256_setzero_ps();
    __m256 s_normA=_mm256_setzero_ps();
    __m256 s_normB=_mm256_setzero_ps();
    const float*b_ptr=b.data();
    for(;i<=dim-8;i+=8){
        __m256 va=_mm256_load_ps(a+i);
        __m256 vb=_mm256_loadu_ps(b_ptr+i);
        s_dot=_mm256_fmadd_ps(va,vb,s_dot);
        s_normA=_mm256_fmadd_ps(va,va,s_normA);
        s_normB=_mm256_fmadd_ps(vb,vb,s_normB);
    }
    dot=sum256(s_dot);
    normA=sum256(s_normA);
    normB=sum256(s_normB);
    for(;i<dim;i++){
        dot+=a[i]*b_ptr[i];
        normA+=a[i]*a[i];
        normB+=b_ptr[i]*b_ptr[i];
    }
    if(normA==0||normB==0){
        return 0;
    }
    return dot/(sqrt(normA)*sqrt(normB));
}

float Engine::compute_distance(const float*a,const vector<float>&b) const{
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
    if(id_to_uuid.size()==0){
        return {};
    }
    if(query.size()!=dim){
        return {};
    }
    priority_queue<pair<float,int>> pq;
    int n=id_to_uuid.size();
    for(int i=0;i<n;i++){
        float dist=compute_distance(&flat_vectors[i*dim],query);
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
    int n=id_to_uuid.size();
    file.write(reinterpret_cast<const char*>(&n),sizeof(n));
    if(n==0){
        return;
    }
    file.write(reinterpret_cast<const char*>(&dim),sizeof(dim));
    for(int i=0;i<n;i++){
        int uuid_len=id_to_uuid[i].size();
        file.write(reinterpret_cast<const char*>(&uuid_len),sizeof(uuid_len));
        file.write(id_to_uuid[i].data(),uuid_len);
        file.write(reinterpret_cast<const char*>(&flat_vectors[i*dim]),dim*sizeof(float));
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
    flat_vectors.clear();
    metadata.clear();
    uuid_to_id.clear();
    id_to_uuid.clear();
    int m_type=0;
    file.read(reinterpret_cast<char*>(&m_type),sizeof(m_type));
    metric=static_cast<MetricType>(m_type);
    int n=0;
    file.read(reinterpret_cast<char*>(&n),sizeof(n));
    if(n==0){
        dim=0;
        file.close();
        return;
    }
    file.read(reinterpret_cast<char*>(&dim),sizeof(dim));
    for(int i=0;i<n;i++){
        int uuid_len=0;
        file.read(reinterpret_cast<char*>(&uuid_len),sizeof(uuid_len));
        string uuid(uuid_len,'\0');
        file.read(&uuid[0],uuid_len);
        int newid=id_to_uuid.size();
        uuid_to_id[uuid]=newid;
        id_to_uuid.push_back(uuid);
        vector<float> vec(dim);
        file.read(reinterpret_cast<char*>(vec.data()),dim*sizeof(float));
        flat_vectors.insert(flat_vectors.end(),vec.begin(),vec.end());
        int meta_len=0;
        file.read(reinterpret_cast<char*>(&meta_len),sizeof(meta_len));
        string meta(meta_len,'\0');
        file.read(&meta[0],meta_len);
        metadata.push_back(meta);
    }
    file.close();
}