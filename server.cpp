#include"database.hpp"
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

string extract_json_string(const string&json,const string&key){
    string k="\""+key+"\"";
    auto pos=json.find(k);
    if(pos==string::npos){
        return "";
    }
    pos=json.find(":",pos);
    if(pos==string::npos){
        return "";
    }
    auto start=json.find("\"",pos);
    if(start==string::npos){
        return "";
    }
    auto end=json.find("\"",start+1);
    if(end==string::npos){
        return "";
    }
    return json.substr(start+1,end-start-1);
}

vector<float> extract_json_vector(const string&json,const string&key){
    vector<float> res;
    string k="\""+key+"\"";
    auto pos=json.find(k);
    if(pos==string::npos){
        return res;
    }
    pos=json.find(":",pos);
    if(pos==string::npos){
        return res;
    }
    auto start=json.find("[",pos);
    if(start==string::npos){
        return res;
    }
    auto end=json.find("]",start);
    if(end==string::npos){
        return res;
    }
    string arr=json.substr(start+1,end-start-1);
    stringstream ss(arr);
    string token;
    while(getline(ss,token,',')){
        res.push_back(stof(token));
    }
    return res;
}

int extract_json_int(const string&json,const string&key){
    string k="\""+key+"\"";
    auto pos=json.find(k);
    if(pos==string::npos){
        return 0;
    }
    pos=json.find(":",pos);
    if(pos==string::npos){
        return 0;
    }
    auto end=json.find_first_of(",}",pos);
    if(end==string::npos){
        return 0;
    }
    string val=json.substr(pos+1,end-pos-1);
    return stoi(val);
}

void handle_client(int client_socket,Engine&db){
    char buffer[4096]={0};
    read(client_socket,buffer,4095);
    string req(buffer);
    string res="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
    auto body_pos=req.find("\r\n\r\n");
    string body="";
    if(body_pos!=string::npos){
        body=req.substr(body_pos+4);
    }
    if(req.find("POST /insert")!=string::npos){
        string uuid=extract_json_string(body,"uuid");
        string meta=extract_json_string(body,"metadata");
        vector<float> emb=extract_json_vector(body,"vector");
        db.insert(uuid,emb,meta);
        res+="{\"status\":\"success\"}";
    }
    else if(req.find("POST /search")!=string::npos){
        vector<float> query=extract_json_vector(body,"query");
        int k=extract_json_int(body,"k");
        auto results=db.search(query,k);
        res+="{\"results\":[";
        for(int i=0;i<results.size();i++){
            res+="{\"uuid\":\""+results[i].uuid+"\",\"dist\":"+to_string(results[i].dist)+",\"metadata\":\""+results[i].metadata+"\"}";
            if(i!=results.size()-1){
                res+=",";
            }
        }
        res+="]}";
    }
    else if(req.find("POST /update")!=string::npos){
        string uuid=extract_json_string(body,"uuid");
        string meta=extract_json_string(body,"metadata");
        vector<float> emb=extract_json_vector(body,"vector");
        db.update(uuid,emb,meta);
        res+="{\"status\":\"success\"}";
    }
    else if(req.find("POST /delete")!=string::npos){
        string uuid=extract_json_string(body,"uuid");
        db.remove(uuid);
        res+="{\"status\":\"success\"}";
    }
    else{
        res="HTTP/1.1 404 Not Found\r\n\r\n";
    }
    write(client_socket,res.c_str(),res.size());
    close(client_socket);
}

int main(){
    Engine db;
    int server_fd=socket(AF_INET,SOCK_STREAM,0);
    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    sockaddr_in address;
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=INADDR_ANY;
    address.sin_port=htons(8080);
    bind(server_fd,(struct sockaddr*)&address,sizeof(address));
    listen(server_fd,10);
    while(true){
        int client_socket=accept(server_fd,nullptr,nullptr);
        handle_client(client_socket,db);
    }
    return 0;
}
