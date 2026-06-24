#include"src/database.hpp"
#include<chrono>
#include<fstream>
#include<iostream>
#include<numeric>
#include<algorithm>
#include<iomanip>

using namespace std;

vector<vector<float>> read_fvecs(const string&filename,int&dim,int max_vecs=-1){
    ifstream file(filename,ios::binary);
    if(!file){
        cerr<<"Could not open "<<filename<<endl;
        exit(1);
    }
    vector<vector<float>> vecs;
    int d;
    while(file.read((char*)&d,4)){
        if(dim==0)dim=d;
        vector<float> vec(d);
        file.read((char*)vec.data(),d*sizeof(float));
        vecs.push_back(vec);
        if(max_vecs>0&&vecs.size()>=max_vecs)break;
    }
    return vecs;
}

int main(){
    string base_file="sift/sift_base.fvecs";
    string query_file="sift/sift_query.fvecs";
    
    int dim=0;
    cout<<"LOADING SIFT1M BASE VECTORS..."<<endl;
    vector<vector<float>> base_vecs=read_fvecs(base_file,dim);
    cout<<"Loaded "<<base_vecs.size()<<" vectors of dimension "<<dim<<endl;
    
    int q_dim=0;
    cout<<"LOADING SIFT1M QUERY VECTORS..."<<endl;
    vector<vector<float>> query_vecs=read_fvecs(query_file,q_dim);
    cout<<"Loaded "<<query_vecs.size()<<" queries."<<endl;
    
    Engine db(MetricType::L2);
    db.reserve(base_vecs.size()+1000);
    
    cout<<"STARTING INSERTION..."<<endl;
    auto start_insert=chrono::high_resolution_clock::now();
    for(size_t i=0;i<base_vecs.size();i++){
        db.insert("uuid_"+to_string(i),base_vecs[i],"meta");
        if((i+1)%100000==0){
            cout<<"Inserted "<<(i+1)<<" / "<<base_vecs.size()<<endl;
        }
    }
    auto end_insert=chrono::high_resolution_clock::now();
    double insert_time=chrono::duration<double,milli>(end_insert-start_insert).count();
    
    cout<<"STARTING SEARCH QUERIES..."<<endl;
    vector<double> latencies;
    latencies.reserve(query_vecs.size());
    
    auto start_search=chrono::high_resolution_clock::now();
    for(size_t i=0;i<query_vecs.size();i++){
        auto t1=chrono::high_resolution_clock::now();
        db.search(query_vecs[i],10);
        auto t2=chrono::high_resolution_clock::now();
        latencies.push_back(chrono::duration<double,micro>(t2-t1).count());
    }
    auto end_search=chrono::high_resolution_clock::now();
    double search_time=chrono::duration<double,milli>(end_search-start_search).count();
    
    sort(latencies.begin(),latencies.end());
    
    cout<<"\n==========================================="<<endl;
    cout<<"          SIFT1M BENCHMARK RESULTS         "<<endl;
    cout<<"==========================================="<<endl;
    cout<<"VECTORS: "<<base_vecs.size()<<" | DIMENSION: "<<dim<<" | QUERIES: "<<query_vecs.size()<<endl;
    cout<<"TOTAL INSERT TIME: "<<insert_time<<" ms"<<endl;
    cout<<"TOTAL SEARCH TIME: "<<search_time<<" ms"<<endl;
    cout<<"SEARCH THROUGHPUT: "<<query_vecs.size()/(search_time/1000.0)<<" queries/sec"<<endl;
    cout<<"-------------------------------------------"<<endl;
    cout<<fixed<<setprecision(2);
    cout<<"LATENCY PERCENTILES (Microseconds):"<<endl;
    cout<<"  Min:     "<<latencies.front()<<" us"<<endl;
    cout<<"  Avg:     "<<accumulate(latencies.begin(),latencies.end(),0.0)/latencies.size()<<" us"<<endl;
    cout<<"  Median:  "<<latencies[latencies.size()/2]<<" us"<<endl;
    cout<<"  p90:     "<<latencies[latencies.size()*0.90]<<" us"<<endl;
    cout<<"  p95:     "<<latencies[latencies.size()*0.95]<<" us"<<endl;
    cout<<"  p99:     "<<latencies[latencies.size()*0.99]<<" us"<<endl;
    cout<<"  p99.9:   "<<latencies[latencies.size()*0.999]<<" us"<<endl;
    cout<<"  Max:     "<<latencies.back()<<" us"<<endl;
    cout<<"==========================================="<<endl;
    
    return 0;
}
