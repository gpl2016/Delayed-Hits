//
// Created by dell on 2021/7/12.
//
#include <iostream>
#include <unordered_map>
#include <map>
using namespace std;
unordered_map<string,string> test_map;
unordered_map<string,string> test_map_new;

bool setFetchingtoIn(const  std::string & flow_id){
    //assert(test_map.find(flow_id)->second=="fetching");
    test_map.erase(flow_id);
    return test_map.insert({flow_id,"in"}).second;

}

int main() {

    string key[100];
    key[0]="aaa";
    key[1]="nnnn";
    key[2]="hufweeuifwe";
    test_map.insert({"testtest","test"});
   // setFetchingtoIn("testtest");
    cout<<test_map.erase("dwe")<<endl;
    cout<<test_map.insert({"111","out"}).second<<endl;
    test_map.insert({key[0],"out"});
    if(test_map.find(key[0])!=test_map.end())
        test_map.erase(key[0]);
    test_map.insert({key[0],"occupy"});
    test_map.erase(key[0]);
    test_map.insert({key[1],"in"});
    cout<<(test_map.find("aaa")==test_map_new.end())<<endl;
    if(test_map.count("aaa")==0)

        cout<<"out"<<endl;
    for(auto &p:test_map)
        cout<<p.first<<" "<<p.second<<endl;

}




