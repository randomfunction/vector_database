#include"src/database.hpp"
#include<chrono>
#include<random>
#include<numeric>
#include<algorithm>
#include<iomanip>

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
    vector<double> latencies;
    latencies.reserve(num_queries);
    
    auto start_total=chrono::high_resolution_clock::now();
    for(int i=0;i<num_queries;i++){
        auto t1=chrono::high_resolution_clock::now();
        db.search(queries[i],10);
        auto t2=chrono::high_resolution_clock::now();
        double span=chrono::duration<double,micro>(t2-t1).count();
        latencies.push_back(span);
    }
    auto end_total=chrono::high_resolution_clock::now();
    
    double insert_time=chrono::duration<double,milli>(end_insert-start_insert).count();
    double total_time=chrono::duration<double,milli>(end_total-start_total).count();
    
    sort(latencies.begin(),latencies.end());
    
    double min_lat=latencies.front();
    double max_lat=latencies.back();
    double median_lat=latencies[latencies.size()/2];
    double p90_lat=latencies[latencies.size()*0.90];
    double p95_lat=latencies[latencies.size()*0.95];
    double p99_lat=latencies[latencies.size()*0.99];
    double p999_lat=latencies[latencies.size()*0.999];
    
    double sum=accumulate(latencies.begin(),latencies.end(),0.0);
    double avg_lat=sum/latencies.size();
    
    cout<<"\n==========================================="<<endl;
    cout<<"           FINAL BENCHMARK RESULTS           "<<endl;
    cout<<"==========================================="<<endl;
    cout<<"VECTORS: "<<num_vectors<<" | DIMENSION: "<<dim<<" | QUERIES: "<<num_queries<<endl;
    cout<<"TOTAL INSERT TIME: "<<insert_time<<" ms"<<endl;
    cout<<"TOTAL SEARCH TIME: "<<total_time<<" ms"<<endl;
    cout<<"SEARCH THROUGHPUT: "<<num_queries/(total_time/1000.0)<<" queries/sec"<<endl;
    cout<<"-------------------------------------------"<<endl;
    cout<<fixed<<setprecision(2);
    cout<<"LATENCY PERCENTILES (Microseconds):"<<endl;
    cout<<"  Min:     "<<min_lat<<" us"<<endl;
    cout<<"  Avg:     "<<avg_lat<<" us"<<endl;
    cout<<"  Median:  "<<median_lat<<" us"<<endl;
    cout<<"  p90:     "<<p90_lat<<" us"<<endl;
    cout<<"  p95:     "<<p95_lat<<" us"<<endl;
    cout<<"  p99:     "<<p99_lat<<" us"<<endl;
    cout<<"  p99.9:   "<<p999_lat<<" us"<<endl;
    cout<<"  Max:     "<<max_lat<<" us"<<endl;
    cout<<"==========================================="<<endl;
    
    return 0;
}
