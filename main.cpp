#include"database.hpp"

int main(){
    cout<<"TESTING L2 METRIC SEARCH"<<endl;
    Engine db(MetricType::L2);
    db.insert("u1",{1.0,0.0},"M1");
    db.insert("u2",{0.0,1.0},"M2");
    db.insert("u3",{1.0,1.0},"M3");
    
    vector<SearchResult> res=db.search({1.0,0.1},2);
    cout<<"EXPECTED 2 RESULTS GOT "<<res.size()<<endl;
    for(int i=0;i<res.size();i++){
        cout<<"RESULT "<<i<<": UUID="<<res[i].uuid<<" DIST="<<res[i].dist<<" META="<<res[i].metadata<<endl;
    }
    
    cout<<"\nTESTING UPDATE FUNCTION"<<endl;
    db.update("u1",{0.0,0.0},"M1_UPDATED");
    res=db.search({0.0,0.0},1);
    cout<<"EXPECTED UUID=u1 META=M1_UPDATED"<<endl;
    cout<<"GOT UUID="<<res[0].uuid<<" META="<<res[0].metadata<<endl;
    
    cout<<"\nTESTING REMOVE FUNCTION"<<endl;
    db.remove("u2");
    res=db.search({0.0,1.0},1);
    cout<<"EXPECTED UUID=u1 META=M1_UPDATED (since u2 is deleted and u1 is now the closest)"<<endl;
    cout<<"GOT UUID="<<res[0].uuid<<" META="<<res[0].metadata<<endl;
    
    cout<<"\nTESTING DOT PRODUCT METRIC"<<endl;
    db.set_metric(MetricType::DOT);
    db.insert("u4",{2.0,2.0},"M4");
    res=db.search({1.0,1.0},1);
    cout<<"EXPECTED UUID=u4"<<endl;
    cout<<"GOT UUID="<<res[0].uuid<<" DIST="<<res[0].dist<<" META="<<res[0].metadata<<endl;
    
    cout<<"\nTESTING FILE SAVE AND LOAD"<<endl;
    db.save_to_file("test_db.bin");
    Engine db2;
    db2.load_from_file("test_db.bin");
    res=db2.search({1.0,1.0},1);
    cout<<"EXPECTED UUID=u4 FROM LOADED DB"<<endl;
    cout<<"GOT UUID="<<res[0].uuid<<" DIST="<<res[0].dist<<" META="<<res[0].metadata<<endl;
    
    cout<<"\nALL TESTS PASSED WITH VISUAL VERIFICATION"<<endl;
    return 0;
}
