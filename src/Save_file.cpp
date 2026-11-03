#include"database.hpp"
#include<iostream>
#include<fstream>
#include<sstream>
#include<array>
#include<cstdint>
#include<cstring>
#include<stdexcept>

using namespace std;

// Snapshot binary format
// -----------------------
//   magic[4]      = "VDBF"
//   version       = uint32  (bump when the body layout changes)
//   body_len      = uint64  (bytes in the body section)
//   crc32         = uint32  (CRC-32 of the body section)
//   body[body_len]= serialized engine state
//
// The body is written in host byte order (little-endian on the x86/ARM targets
// we support). The header lets us reject a file that is not a snapshot or is
// from an incompatible version, and the CRC catches truncation/corruption
// before any of it is parsed into the graph.
static const char     SNAPSHOT_MAGIC[4] = {'V','D','B','F'};
static const uint32_t SNAPSHOT_VERSION  = 1;

static uint32_t crc32_bytes(const char* data,size_t len){
    // zlib polynomial, table built once (thread-safe static init).
    static const array<uint32_t,256> table=[]{
        array<uint32_t,256> t{};
        for(uint32_t i=0;i<256;i++){
            uint32_t c=i;
            for(int k=0;k<8;k++){
                c=(c&1u)?(0xEDB88320u^(c>>1)):(c>>1);
            }
            t[i]=c;
        }
        return t;
    }();

    uint32_t crc=0xFFFFFFFFu;
    for(size_t i=0;i<len;i++){
        crc=table[(crc^static_cast<unsigned char>(data[i]))&0xFFu]^(crc>>8);
    }
    return crc^0xFFFFFFFFu;
}

void Engine::save_to_file(const string&filename){
    shared_lock<shared_mutex> lock(rw_lock);

    // Serialize the body into memory first so we can checksum it.
    ostringstream body(ios::out|ios::binary);

    int m_type=static_cast<int>(metric);
    body.write(reinterpret_cast<const char*>(&m_type),sizeof(m_type));
    int n=id_to_uuid.size();
    body.write(reinterpret_cast<const char*>(&n),sizeof(n));
    body.write(reinterpret_cast<const char*>(&dim),sizeof(dim));
    body.write(reinterpret_cast<const char*>(&ep_id),sizeof(ep_id));
    body.write(reinterpret_cast<const char*>(&max_layer),sizeof(max_layer));

    for(int i=0;i<n;i++){
        int uuid_len=id_to_uuid[i].size();
        body.write(reinterpret_cast<const char*>(&uuid_len),sizeof(uuid_len));
        body.write(id_to_uuid[i].data(),uuid_len);
        body.write(reinterpret_cast<const char*>(&flat_vectors[i*dim]),dim*sizeof(float));
        int meta_len=metadata[i].size();
        body.write(reinterpret_cast<const char*>(&meta_len),sizeof(meta_len));
        body.write(metadata[i].data(),meta_len);
        bool act=active[i];
        body.write(reinterpret_cast<const char*>(&act),sizeof(act));
        int max_l=nodes[i].max_layer;
        body.write(reinterpret_cast<const char*>(&max_l),sizeof(max_l));
        int edge_off=nodes[i].edge_offset;
        body.write(reinterpret_cast<const char*>(&edge_off),sizeof(edge_off));
    }

    int edges_size=flat_edges.size();
    body.write(reinterpret_cast<const char*>(&edges_size),sizeof(edges_size));
    if(edges_size>0){
        body.write(reinterpret_cast<const char*>(flat_edges.data()),edges_size*sizeof(int));
    }

    const string bytes=body.str();
    uint64_t body_len=bytes.size();
    uint32_t crc=crc32_bytes(bytes.data(),bytes.size());

    ofstream file(filename,ios::out|ios::binary);
    if(!file.is_open()){
        throw runtime_error("save_to_file: cannot open '"+filename+"' for writing");
    }
    file.write(SNAPSHOT_MAGIC,4);
    file.write(reinterpret_cast<const char*>(&SNAPSHOT_VERSION),sizeof(SNAPSHOT_VERSION));
    file.write(reinterpret_cast<const char*>(&body_len),sizeof(body_len));
    file.write(reinterpret_cast<const char*>(&crc),sizeof(crc));
    file.write(bytes.data(),bytes.size());
    if(!file){
        throw runtime_error("save_to_file: write failed for '"+filename+"'");
    }
    file.close();
}

void Engine::load_from_file(const string&filename){
    unique_lock<shared_mutex> lock(rw_lock);
    ifstream file(filename,ios::in|ios::binary);
    if(!file.is_open()){
        throw runtime_error("load_from_file: cannot open '"+filename+"'");
    }

    char magic[4]={0,0,0,0};
    file.read(magic,4);
    if(file.gcount()!=4||memcmp(magic,SNAPSHOT_MAGIC,4)!=0){
        throw runtime_error("load_from_file: '"+filename+"' is not a snapshot file (bad magic)");
    }
    uint32_t version=0;
    file.read(reinterpret_cast<char*>(&version),sizeof(version));
    if(version!=SNAPSHOT_VERSION){
        throw runtime_error("load_from_file: unsupported snapshot version "+to_string(version)+
                            " (expected "+to_string(SNAPSHOT_VERSION)+")");
    }
    uint64_t body_len=0;
    file.read(reinterpret_cast<char*>(&body_len),sizeof(body_len));
    uint32_t crc_expected=0;
    file.read(reinterpret_cast<char*>(&crc_expected),sizeof(crc_expected));
    if(!file){
        throw runtime_error("load_from_file: truncated header in '"+filename+"'");
    }

    string body(body_len,'\0');
    if(body_len>0){
        file.read(&body[0],body_len);
        if(static_cast<uint64_t>(file.gcount())!=body_len){
            throw runtime_error("load_from_file: truncated body in '"+filename+"'");
        }
    }
    if(crc32_bytes(body.data(),body.size())!=crc_expected){
        throw runtime_error("load_from_file: checksum mismatch in '"+filename+"' (corrupt file)");
    }

    istringstream in(body,ios::in|ios::binary);

    flat_vectors.clear();
    metadata.clear();
    uuid_to_id.clear();
    id_to_uuid.clear();
    active.clear();
    nodes.clear();
    flat_edges.clear();

    int m_type=0;
    in.read(reinterpret_cast<char*>(&m_type),sizeof(m_type));
    metric=static_cast<MetricType>(m_type);
    int n=0;
    in.read(reinterpret_cast<char*>(&n),sizeof(n));
    in.read(reinterpret_cast<char*>(&dim),sizeof(dim));
    in.read(reinterpret_cast<char*>(&ep_id),sizeof(ep_id));
    in.read(reinterpret_cast<char*>(&max_layer),sizeof(max_layer));

    if(n<0||dim<0){
        throw runtime_error("load_from_file: invalid header values in '"+filename+"'");
    }
    if(n==0){
        dim=0;
        ep_id=-1;
        max_layer=-1;
        return;
    }

    for(int i=0;i<n;i++){
        int uuid_len=0;
        in.read(reinterpret_cast<char*>(&uuid_len),sizeof(uuid_len));
        string uuid(uuid_len,'\0');
        in.read(&uuid[0],uuid_len);
        int newid=id_to_uuid.size();
        uuid_to_id[uuid]=newid;
        id_to_uuid.push_back(uuid);
        vector<float> vec(dim);
        in.read(reinterpret_cast<char*>(vec.data()),dim*sizeof(float));
        flat_vectors.insert(flat_vectors.end(),vec.begin(),vec.end());
        int meta_len=0;
        in.read(reinterpret_cast<char*>(&meta_len),sizeof(meta_len));
        string meta(meta_len,'\0');
        in.read(&meta[0],meta_len);
        metadata.push_back(meta);
        bool act=false;
        in.read(reinterpret_cast<char*>(&act),sizeof(act));
        active.push_back(act);
        HNSWNode node;
        node.id=newid;
        in.read(reinterpret_cast<char*>(&node.max_layer),sizeof(node.max_layer));
        in.read(reinterpret_cast<char*>(&node.edge_offset),sizeof(node.edge_offset));
        nodes.push_back(node);
    }

    int edges_size=0;
    in.read(reinterpret_cast<char*>(&edges_size),sizeof(edges_size));
    if(edges_size<0){
        throw runtime_error("load_from_file: invalid edge count in '"+filename+"'");
    }
    if(edges_size>0){
        flat_edges.resize(edges_size);
        in.read(reinterpret_cast<char*>(flat_edges.data()),edges_size*sizeof(int));
    }
    if(!in){
        throw runtime_error("load_from_file: body parse underrun in '"+filename+"'");
    }
}
