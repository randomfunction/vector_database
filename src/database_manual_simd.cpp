#include"database.hpp"
#include<random>

Engine::Engine(MetricType m){
    metric=m;
    dim=0;
    mult=1.0/log(1.0*M);
    ep_id=-1;
    max_layer=-1;
}

void Engine::set_metric(MetricType m){
    unique_lock<shared_mutex> lock(rw_lock);
    metric=m;
}

float Engine::sum256(__m256 x) const{
    float arr[8];
    _mm256_storeu_ps(arr,x);
    return arr[0]+arr[1]+arr[2]+arr[3]+arr[4]+arr[5]+arr[6]+arr[7];
}

float Engine::squared_eucledian_distance(const float*a,const float*b) const{
    float res=0;
    int i=0;
    __m256 sum=_mm256_setzero_ps();
    for(;i<=dim-8;i+=8){
        __m256 va=_mm256_load_ps(a+i);
        __m256 vb=_mm256_loadu_ps(b+i);
        __m256 diff=_mm256_sub_ps(va,vb);
        sum=_mm256_fmadd_ps(diff,diff,sum);
    }
    res=sum256(sum);
    for(;i<dim;i++){
        res+=(a[i]-b[i])*(a[i]-b[i]);
    }
    return res;
}

float Engine::dot_product(const float*a,const float*b) const{
    float ans=0;
    int i=0;
    __m256 sum=_mm256_setzero_ps();
    for(;i<=dim-8;i+=8){
        __m256 va=_mm256_load_ps(a+i);
        __m256 vb=_mm256_loadu_ps(b+i);
        sum=_mm256_fmadd_ps(va,vb,sum);
    }
    ans=sum256(sum);
    for(;i<dim;i++){
        ans+=a[i]-b[i];
    }
    return ans;
}

float Engine::cosine_similarity(const float*a,const float*b) const{
    float dot=0,normA=0,normB=0;
    int i=0;
    __m256 s_dot=_mm256_setzero_ps();
    __m256 s_normA=_mm256_setzero_ps();
    __m256 s_normB=_mm256_setzero_ps();
    for(;i<=dim-8;i+=8){
        __m256 va=_mm256_load_ps(a+i);
        __m256 vb=_mm256_loadu_ps(b+i);
        s_dot=_mm256_fmadd_ps(va,vb,s_dot);
        s_normA=_mm256_fmadd_ps(va,va,s_normA);
        s_normB=_mm256_fmadd_ps(vb,vb,s_normB);
    }
    dot=sum256(s_dot);
    normA=sum256(s_normA);
    normB=sum256(s_normB);
    for(;i<dim;i++){
        dot+=a[i]*b[i];
        normA+=a[i]*a[i];
        normB+=b[i]*b[i];
    }
    if(normA==0||normB==0){
        return 0;
    }
    return dot/(sqrt(normA)*sqrt(normB));
}

float Engine::compute_distance(const float*a,const float*b) const{
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

priority_queue<pair<float,int>> Engine::search_layer(const float*query,int entry_point_id,int ef,int layer){
    priority_queue<pair<float,int>> top_candidates;
    priority_queue<pair<float,int>,vector<pair<float,int>>,greater<pair<float,int>>> candidates;
    unordered_set<int> visited;
    float ep_dist=compute_distance(&flat_vectors[entry_point_id*dim],query);
    candidates.push({ep_dist,entry_point_id});
    top_candidates.push({ep_dist,entry_point_id});
    visited.insert(entry_point_id);
    while(!candidates.empty()){
        auto [c_dist,c_id]=candidates.top();
        candidates.pop();
        if(c_dist>top_candidates.top().first){
            break;
        }
        for(int neighbor_id:nodes[c_id].neighbors[layer]){
            if(visited.find(neighbor_id)==visited.end()){
                visited.insert(neighbor_id);
                float n_dist=compute_distance(&flat_vectors[neighbor_id*dim],query);
                if(top_candidates.size()<ef||n_dist<top_candidates.top().first){
                    candidates.push({n_dist,neighbor_id});
                    top_candidates.push({n_dist,neighbor_id});
                    if(top_candidates.size()>ef){
                        top_candidates.pop();
                    }
                }
            }
        }
    }
    return top_candidates;
}

void Engine::prune_connections(int node_id,int layer,int max_conn){
    if(nodes[node_id].neighbors[layer].size()<=max_conn){
        return;
    }
    priority_queue<pair<float,int>> pq;
    for(int neighbor_id:nodes[node_id].neighbors[layer]){
        float dist=compute_distance(&flat_vectors[node_id*dim],&flat_vectors[neighbor_id*dim]);
        pq.push({dist,neighbor_id});
        if(pq.size()>max_conn){
            pq.pop();
        }
    }
    nodes[node_id].neighbors[layer].clear();
    while(!pq.empty()){
        nodes[node_id].neighbors[layer].push_back(pq.top().second);
        pq.pop();
    }
}

void Engine::insert(const string&uuid,const vector<float>&emb,const string&data){
    unique_lock<shared_mutex> lock(rw_lock);
    if(uuid_to_id.count(uuid)){
        int id=uuid_to_id[uuid];
        if(!active[id]){
            active[id]=true;
            for(int i=0;i<dim;i++){
                flat_vectors[id*dim+i]=emb[i];
            }
            metadata[id]=data;
            return;
        }
        return;
    }
    if(dim==0){
        dim=emb.size();
    }
    else if(emb.size()!=dim){
        return;
    }
    int new_id=id_to_uuid.size();
    uuid_to_id[uuid]=new_id;
    id_to_uuid.push_back(uuid);
    flat_vectors.insert(flat_vectors.end(),emb.begin(),emb.end());
    metadata.push_back(data);
    active.push_back(true);
    
    static default_random_engine generator(42);
    static uniform_real_distribution<double> distribution(0.0,1.0);
    double r=distribution(generator);
    if(r==0.0)r=0.00001;
    int l=(int)(-log(r)*mult);
    
    HNSWNode new_node;
    new_node.id=new_id;
    new_node.max_layer=l;
    new_node.neighbors.resize(l+1);
    
    if(nodes.empty()){
        ep_id=new_id;
        max_layer=l;
        nodes.push_back(new_node);
        return;
    }
    
    int curr_ep=ep_id;
    int curr_max_layer=max_layer;
    
    for(int level=curr_max_layer;level>l;level--){
        auto best=search_layer(emb.data(),curr_ep,1,level);
        curr_ep=best.top().second;
    }
    
    for(int level=min(l,curr_max_layer);level>=0;level--){
        auto neighbors_queue=search_layer(emb.data(),curr_ep,ef_construction,level);
        int max_conn=(level==0)?M_max_0:M;
        vector<int> selected_neighbors;
        while(!neighbors_queue.empty()&&selected_neighbors.size()<max_conn){
            selected_neighbors.push_back(neighbors_queue.top().second);
            neighbors_queue.pop();
        }
        new_node.neighbors[level]=selected_neighbors;
        for(int n_id:selected_neighbors){
            nodes[n_id].neighbors[level].push_back(new_id);
            if(nodes[n_id].neighbors[level].size()>max_conn){
                prune_connections(n_id,level,max_conn);
            }
        }
        curr_ep=selected_neighbors.front();
    }
    
    if(l>max_layer){
        max_layer=l;
        ep_id=new_id;
    }
    nodes.push_back(new_node);
}

vector<SearchResult> Engine::search(const vector<float>&query,int k){
    shared_lock<shared_mutex> lock(rw_lock);
    if(nodes.empty()||query.size()!=dim){
        return {};
    }
    int curr_ep=ep_id;
    for(int l=max_layer;l>0;l--){
        auto best=search_layer(query.data(),curr_ep,1,l);
        curr_ep=best.top().second;
    }
    int ef=max(ef_search,k);
    auto top_k_queue=search_layer(query.data(),curr_ep,ef,0);
    vector<pair<float,int>> temp;
    while(!top_k_queue.empty()){
        temp.push_back(top_k_queue.top());
        top_k_queue.pop();
    }
    reverse(temp.begin(),temp.end());
    vector<SearchResult> res;
    for(auto p:temp){
        if(active[p.second]){
            float real_dist=(metric==MetricType::L2)?p.first:-p.first;
            res.push_back({id_to_uuid[p.second],real_dist,metadata[p.second]});
            if(res.size()==k){
                break;
            }
        }
    }
    return res;
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
    active[id]=true;
}

void Engine::remove(const string&uuid){
    unique_lock<shared_mutex> lock(rw_lock);
    if(!uuid_to_id.count(uuid)){
        return;
    }
    int id=uuid_to_id[uuid];
    active[id]=false;
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
    file.write(reinterpret_cast<const char*>(&ep_id),sizeof(ep_id));
    file.write(reinterpret_cast<const char*>(&max_layer),sizeof(max_layer));
    for(int i=0;i<n;i++){
        int uuid_len=id_to_uuid[i].size();
        file.write(reinterpret_cast<const char*>(&uuid_len),sizeof(uuid_len));
        file.write(id_to_uuid[i].data(),uuid_len);
        file.write(reinterpret_cast<const char*>(&flat_vectors[i*dim]),dim*sizeof(float));
        int meta_len=metadata[i].size();
        file.write(reinterpret_cast<const char*>(&meta_len),sizeof(meta_len));
        file.write(metadata[i].data(),meta_len);
        bool act=active[i];
        file.write(reinterpret_cast<const char*>(&act),sizeof(act));
        int max_l=nodes[i].max_layer;
        file.write(reinterpret_cast<const char*>(&max_l),sizeof(max_l));
        for(int l=0;l<=max_l;l++){
            int num_neigh=nodes[i].neighbors[l].size();
            file.write(reinterpret_cast<const char*>(&num_neigh),sizeof(num_neigh));
            if(num_neigh>0){
                file.write(reinterpret_cast<const char*>(nodes[i].neighbors[l].data()),num_neigh*sizeof(int));
            }
        }
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
    active.clear();
    nodes.clear();
    int m_type=0;
    file.read(reinterpret_cast<char*>(&m_type),sizeof(m_type));
    metric=static_cast<MetricType>(m_type);
    int n=0;
    file.read(reinterpret_cast<char*>(&n),sizeof(n));
    if(n==0){
        dim=0;
        ep_id=-1;
        max_layer=-1;
        file.close();
        return;
    }
    file.read(reinterpret_cast<char*>(&dim),sizeof(dim));
    file.read(reinterpret_cast<char*>(&ep_id),sizeof(ep_id));
    file.read(reinterpret_cast<char*>(&max_layer),sizeof(max_layer));
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
        bool act=false;
        file.read(reinterpret_cast<char*>(&act),sizeof(act));
        active.push_back(act);
        HNSWNode node;
        node.id=newid;
        file.read(reinterpret_cast<char*>(&node.max_layer),sizeof(node.max_layer));
        node.neighbors.resize(node.max_layer+1);
        for(int l=0;l<=node.max_layer;l++){
            int num_neigh=0;
            file.read(reinterpret_cast<char*>(&num_neigh),sizeof(num_neigh));
            if(num_neigh>0){
                node.neighbors[l].resize(num_neigh);
                file.read(reinterpret_cast<char*>(node.neighbors[l].data()),num_neigh*sizeof(int));
            }
        }
        nodes.push_back(node);
    }
    file.close();
}