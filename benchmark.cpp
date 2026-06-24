#include"src/database.hpp"
#include<chrono>
#include<random>

int main(){
    int num_vectors=20000;
    int dim=256;
    int num_queries=1000;
    
    cout<<"GENERATING "<<num_vectors<<" VECTORS OF DIMENSION "<<dim<<"..."<<endl;
    mt19937 gen(42);
    uniform_real_distribution<float> dist(-1.0,1.0);
    
    vector<vector<float>> db_vectors(num_vectors,vector<float>(dim));
    for(int i=0;i<num_vectors;i++){
        for(int j=0;j<dim;j++){
            db_vectors[i][j]=dist(gen);
        }
    }
    
    vector<vector<float>> queries(num_queries,vector<float>(dim));
    for(int i=0;i<num_queries;i++){
        for(int j=0;j<dim;j++){
            queries[i][j]=dist(gen);
        }
    }
    
    Engine db(MetricType::COSINE);
    
    cout<<"STARTING INSERTION..."<<endl;
    auto start_insert=chrono::high_resolution_clock::now();
    for(int i=0;i<num_vectors;i++){
        db.insert("uuid_"+to_string(i),db_vectors[i],"meta");
    }
    auto end_insert=chrono::high_resolution_clock::now();
    
    cout<<"STARTING SEARCH QUERIES..."<<endl;
    auto start_search=chrono::high_resolution_clock::now();
    for(int i=0;i<num_queries;i++){
        db.search(queries[i],10);
    }
    auto end_search=chrono::high_resolution_clock::now();
    
    double insert_time=chrono::duration<double,milli>(end_insert-start_insert).count();
    double search_time=chrono::duration<double,milli>(end_search-start_search).count();
    
    cout<<"\n=== BENCHMARK RESULTS ==="<<endl;
    cout<<"VECTORS: "<<num_vectors<<" | DIMENSION: "<<dim<<" | QUERIES: "<<num_queries<<endl;
    cout<<"TOTAL INSERT TIME: "<<insert_time<<" ms"<<endl;
    cout<<"TOTAL SEARCH TIME: "<<search_time<<" ms"<<endl;
    cout<<"AVERAGE SEARCH LATENCY: "<<search_time/num_queries<<" ms/query"<<endl;
    cout<<"SEARCH THROUGHPUT: "<<num_queries/(search_time/1000.0)<<" queries/sec"<<endl;
    
    return 0;
}
