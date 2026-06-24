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

void Engine::reserve(size_t max_elements){
    unique_lock<shared_mutex> lock(rw_lock);
    max_capacity=max_elements;
    metadata.reserve(max_elements);
    active.reserve(max_elements);
    nodes.reserve(max_elements);
    id_to_uuid.reserve(max_elements);
    uuid_to_id.reserve(max_elements);
    size_t avg_edges=(M_max_0+2)+2*(M+2);
    flat_edges.reserve(max_elements*avg_edges);
    if(dim>0){
        flat_vectors.reserve(max_elements*dim);
    }
}

float Engine::squared_eucledian_distance(const float*a,const float*b) const{
    float res=0;
    for(int i=0;i<dim;i++){
        res+=(a[i]-b[i])*(a[i]-b[i]);
    }
    return res;
}

float Engine::dot_product(const float*a,const float*b) const{
    float ans=0;
    for(int i=0;i<dim;i++){
        ans+=a[i]*b[i];
    }
    return ans;
}

float Engine::cosine_similarity(const float*a,const float*b) const{
    float dot=0,normA=0,normB=0;
    for(int i=0;i<dim;i++){
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

int Engine::get_edge_offset(int node_offset,int layer) const{
    if(layer==0)return node_offset;
    return node_offset+(M_max_0+2)+(layer-1)*(M+2);
}

int* Engine::get_edges(int node_id,int layer){
    int offset=get_edge_offset(nodes[node_id].edge_offset,layer);
    return &flat_edges[offset];
}

const int* Engine::get_edges(int node_id,int layer) const{
    int offset=get_edge_offset(nodes[node_id].edge_offset,layer);
    return &flat_edges[offset];
}

void Engine::set_edges(int node_id,int layer,const vector<int>&edges){
    int* ptr=get_edges(node_id,layer);
    ptr[0]=edges.size();
    for(int i=0;i<edges.size();i++){
        ptr[i+1]=edges[i];
    }
}

void Engine::search_layer(const float*query,int entry_point_id,int ef,int layer,vector<pair<float,int>>&top_candidates) const{
    thread_local vector<pair<float,int>> candidates;
    thread_local vector<uint32_t> visited_array;
    thread_local uint32_t visited_version=0;
    if(visited_array.size()<=nodes.size()){
        visited_array.resize(nodes.size()+1000,0);
    }
    visited_version++;
    if(visited_version==0){
        fill(visited_array.begin(),visited_array.end(),0);
        visited_version=1;
    }
    candidates.clear();
    top_candidates.clear();
    float ep_dist=compute_distance(&flat_vectors[entry_point_id*dim],query);
    candidates.push_back({ep_dist,entry_point_id});
    top_candidates.push_back({ep_dist,entry_point_id});
    visited_array[entry_point_id]=visited_version;
    auto comp_greater=[](const pair<float,int>&a,const pair<float,int>&b){return a.first>b.first;};
    while(!candidates.empty()){
        pop_heap(candidates.begin(),candidates.end(),comp_greater);
        auto [c_dist,c_id]=candidates.back();
        candidates.pop_back();
        if(c_dist>top_candidates.front().first){
            break;
        }
        const int* edges=get_edges(c_id,layer);
        int count=edges[0];
        for(int i=1;i<=count;i++){
            int neighbor_id=edges[i];
            if(visited_array[neighbor_id]!=visited_version){
                visited_array[neighbor_id]=visited_version;
                float n_dist=compute_distance(&flat_vectors[neighbor_id*dim],query);
                if(top_candidates.size()<ef||n_dist<top_candidates.front().first){
                    candidates.push_back({n_dist,neighbor_id});
                    push_heap(candidates.begin(),candidates.end(),comp_greater);
                    top_candidates.push_back({n_dist,neighbor_id});
                    push_heap(top_candidates.begin(),top_candidates.end());
                    if(top_candidates.size()>ef){
                        pop_heap(top_candidates.begin(),top_candidates.end());
                        top_candidates.pop_back();
                    }
                }
            }
        }
    }
}

void Engine::prune_connections(int node_id,int layer,int max_conn){
    int* edges=get_edges(node_id,layer);
    int count=edges[0];
    if(count<=max_conn){
        return;
    }
    thread_local vector<pair<float,int>> pq;
    pq.clear();
    for(int i=1;i<=count;i++){
        int neighbor_id=edges[i];
        float dist=compute_distance(&flat_vectors[node_id*dim],&flat_vectors[neighbor_id*dim]);
        pq.push_back({dist,neighbor_id});
        push_heap(pq.begin(),pq.end());
        if(pq.size()>max_conn){
            pop_heap(pq.begin(),pq.end());
            pq.pop_back();
        }
    }
    edges[0]=pq.size();
    for(int i=0;i<pq.size();i++){
        edges[i+1]=pq[i].second;
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
        flat_vectors.reserve(max_capacity*dim);
        if(nodes.capacity()<max_capacity){
            metadata.reserve(max_capacity);
            active.reserve(max_capacity);
            nodes.reserve(max_capacity);
            id_to_uuid.reserve(max_capacity);
            uuid_to_id.reserve(max_capacity);
            size_t avg_edges=(M_max_0+2)+2*(M+2);
            flat_edges.reserve(max_capacity*avg_edges);
        }
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
    new_node.edge_offset=flat_edges.size();
    flat_edges.resize(flat_edges.size()+(M_max_0+2)+l*(M+2),0);
    
    if(nodes.empty()){
        ep_id=new_id;
        max_layer=l;
        nodes.push_back(new_node);
        return;
    }
    
    nodes.push_back(new_node);
    
    int curr_ep=ep_id;
    int curr_max_layer=max_layer;
    thread_local vector<pair<float,int>> top_k_queue;
    
    for(int level=curr_max_layer;level>l;level--){
        search_layer(emb.data(),curr_ep,1,level,top_k_queue);
        curr_ep=top_k_queue.front().second;
    }
    
    for(int level=min(l,curr_max_layer);level>=0;level--){
        search_layer(emb.data(),curr_ep,ef_construction,level,top_k_queue);
        int max_conn=(level==0)?M_max_0:M;
        thread_local vector<int> selected_neighbors;
        selected_neighbors.clear();
        sort(top_k_queue.begin(),top_k_queue.end());
        for(int i=0;i<top_k_queue.size()&&selected_neighbors.size()<max_conn;i++){
            selected_neighbors.push_back(top_k_queue[i].second);
        }
        set_edges(new_id,level,selected_neighbors);
        for(int n_id:selected_neighbors){
            int* n_edges=get_edges(n_id,level);
            n_edges[0]++;
            n_edges[n_edges[0]]=new_id;
            if(n_edges[0]>max_conn){
                prune_connections(n_id,level,max_conn);
            }
        }
        curr_ep=selected_neighbors.front();
    }
    
    if(l>max_layer){
        max_layer=l;
        ep_id=new_id;
    }
}

vector<SearchResult> Engine::search(const vector<float>&query,int k){
    shared_lock<shared_mutex> lock(rw_lock);
    if(nodes.empty()||query.size()!=dim){
        return {};
    }
    int curr_ep=ep_id;
    thread_local vector<pair<float,int>> top_k_queue;
    for(int l=max_layer;l>0;l--){
        search_layer(query.data(),curr_ep,1,l,top_k_queue);
        curr_ep=top_k_queue.front().second;
    }
    int ef=max(ef_search,k);
    search_layer(query.data(),curr_ep,ef,0,top_k_queue);
    sort(top_k_queue.begin(),top_k_queue.end());
    vector<SearchResult> res;
    for(auto p:top_k_queue){
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
        int edge_off=nodes[i].edge_offset;
        file.write(reinterpret_cast<const char*>(&edge_off),sizeof(edge_off));
    }
    int edges_size=flat_edges.size();
    file.write(reinterpret_cast<const char*>(&edges_size),sizeof(edges_size));
    if(edges_size>0){
        file.write(reinterpret_cast<const char*>(flat_edges.data()),edges_size*sizeof(int));
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
    flat_edges.clear();
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
        file.read(reinterpret_cast<char*>(&node.edge_offset),sizeof(node.edge_offset));
        nodes.push_back(node);
    }
    int edges_size=0;
    file.read(reinterpret_cast<char*>(&edges_size),sizeof(edges_size));
    if(edges_size>0){
        flat_edges.resize(edges_size);
        file.read(reinterpret_cast<char*>(flat_edges.data()),edges_size*sizeof(int));
    }
    file.close();
}