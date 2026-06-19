#include"database.hpp"

int main(){
    Engine db(MetricType::L2);
    db.insert("u1",{1.0,0.0},"M1");
    db.insert("u2",{0.0,1.0},"M2");
    db.insert("u3",{1.0,1.0},"M3");
    
    vector<SearchResult> res=db.search({1.0,0.1},2);
    assert(res.size()==2);
    assert(res[0].uuid=="u1");
    
    db.update("u1",{0.0,0.0},"M1_UPDATED");
    res=db.search({0.0,0.0},1);
    assert(res[0].uuid=="u1");
    assert(res[0].metadata=="M1_UPDATED");
    
    db.remove("u2");
    res=db.search({0.0,1.0},1);
    assert(res[0].uuid!="u2");
    
    db.set_metric(MetricType::DOT);
    db.insert("u4",{2.0,2.0},"M4");
    res=db.search({1.0,1.0},1);
    assert(res[0].uuid=="u4");
    
    db.save_to_file("test_db.bin");
    Engine db2;
    db2.load_from_file("test_db.bin");
    res=db2.search({1.0,1.0},1);
    assert(res[0].uuid=="u4");
    
    cout<<"ALL TESTS PASSED"<<endl;
    return 0;
}
