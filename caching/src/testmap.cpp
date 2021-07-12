//
// Created by dell on 2021/7/12.
//
#include <iostream>
#include <unordered_map>
#include <map>
using namespace std;
int main() {
    unordered_map<string,string> test_map;
    string key[100];
    key[0]="aaa";
    key[1]="nnnn";
    key[2]="hufweeuifwe";
    test_map.insert({key[0],"out"});
    if(test_map.find(key[0])!=test_map.end())
        test_map.erase(key[0]);
    test_map.insert({key[0],"occupy"});
    test_map.insert({key[1],"in"});
    for(auto &p:test_map)
        cout<<p.first<<" "<<p.second<<endl;
}




