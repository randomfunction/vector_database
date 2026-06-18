#include<bits/stdc++.h>
using namespace std;

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

    public:
    void insert(const string &uuid,const vector<float> &emb,const string &data){
        if(uuid_to_id.count(uuid)){
            cout<<"UUID ALREADY EXIST"<<endl;
            return;
        }
        int newid= vectors.size();
        uuid_to_id[uuid]=newid;
        id_to_uuid.push_back(uuid);
        vectors.push_back(emb);
        metadata.push_back(data);
    } 

    float squared_eucledian_distance(const vector<float>& a,const vector<float>& b){
        float res=0;
        int n=a.size();
        int m=b.size();
        if(n!=m){
            throw invalid_argument("SIZE OF VECTORS SHOULD BE SAME");
        }
        for(int i=0;i<n;i++){
            res+= (a[i]-b[i])*(a[i]-b[i]);
        }
        return res;
    }


    vector<SearchResult> search(const vector<float>&query, int k){
        if(vectors.size()==0){
            return {};
        }
        if(query.size()!=vectors[0].size()){
            throw invalid_argument("SIZE OF QUERY VECTOR AND DATABASE VECTORS SHOULD BE SAME");
        }
        priority_queue<pair<float,int>> pq;
        for(int i=0;i<vectors.size();i++){
            float dist= squared_eucledian_distance(vectors[i],query);
            pq.push({dist,i});
            if(pq.size()>k) pq.pop();
        }
        vector<SearchResult> res;
        while(!pq.empty()){
            auto [dist, i]=pq.top();
            pq.pop();
            string uuid=id_to_uuid[i];
            string data=metadata[i];
            res.push_back({uuid,dist,data});
        }
        reverse(res.begin(),res.end());
        return res;
    }

    void save_to_file(const string& filename) {
        ofstream file(filename,ios::out|ios::binary);
        if (!file.is_open()) {
            throw runtime_error("Failed to open file for writing: " + filename);
        }

        int n=vectors.size();
        file.write(reinterpret_cast<const char*>(&n), sizeof(n));

        if (n==0) return;

        int dimensions =vectors[0].size();
        file.write(reinterpret_cast<const char*>(&dimensions), sizeof(dimensions));

        for (int i=0; i < n; i++) {
            int uuid_len = id_to_uuid[i].size();
            file.write(reinterpret_cast<const char*>(&uuid_len), sizeof(uuid_len));
            file.write(id_to_uuid[i].data(), uuid_len);
            file.write(reinterpret_cast<const char*>(vectors[i].data()), dimensions * sizeof(float));

            int meta_len = metadata[i].size();
            file.write(reinterpret_cast<const char*>(&meta_len), sizeof(meta_len));
            file.write(metadata[i].data(), meta_len);
        }
        file.close();
    }

    void load_from_file(const string& filename){
        ifstream file(filename, ios::in|ios::binary);
        vectors.clear();
        metadata.clear();
        uuid_to_id.clear();
        int n=0;
        file.read(reinterpret_cast<char*>(&n),sizeof(n));
        if(n==0){
            file.close();
            return;
        }
        int dimensions = 0;
        file.read(reinterpret_cast<char*>(&dimensions), sizeof(dimensions));

        for (int i = 0; i <n;i++) {
            int uuid_len = 0;
            file.read(reinterpret_cast<char*>(&uuid_len), sizeof(uuid_len));
            
            string uuid(uuid_len, '\0');
            file.read(&uuid[0], uuid_len);

            vector<float> vec(dimensions);
            file.read(reinterpret_cast<char*>(vec.data()), dimensions * sizeof(float));

            int meta_len = 0;
            file.read(reinterpret_cast<char*>(&meta_len), sizeof(meta_len));
            
            string meta(meta_len, '\0');
            file.read(&meta[0], meta_len);
            insert(uuid, vec, meta);
        }
    }
};

// int main(){
//     Engine vectordatabase;
    
//     vectordatabase.insert("uuid1",{1.0,0.0,0.0},"Red");
//     vectordatabase.insert("uuid2",{0.0,1.0,0.0},"Green");
//     vectordatabase.insert("uuid3",{1.1,0.0,0.0},"Dark Red");
//     vectordatabase.insert("uuid4",{0.0,0.0,1.0},"Blue");

//     vector<float> query={1.0,0.1,0.0};
//     vector<SearchResult> res=vectordatabase.search(query,2);
//     for(auto it:res){
//         cout<<it.uuid<<" "<<it.dist<<" "<<it.metadata<<endl;
//     }

//     vectordatabase.save_to_file("db.bin");
// }

int main(){
    Engine vectordatabase;
    vectordatabase.load_from_file("db.bin");
    vector<float> query={1.0,0.1,0.0};
    vector<SearchResult> res=vectordatabase.search(query,2);
    for(auto it:res){
        cout<<it.uuid<<" "<<it.dist<<" "<<it.metadata<<endl;
    }
}