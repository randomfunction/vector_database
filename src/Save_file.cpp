#include"database.hpp"
#include<iostream>
#include<fstream>

using namespace std;

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