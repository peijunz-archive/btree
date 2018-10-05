#include <iostream>
#include "btree.h"

using namespace std;

int main(){
    btree<int, 4> bt;
    cout<<bt;
    for(int i=0; i<20; i++){
        bt.insert(i);
    }
    cout<<bt;
    for(int i=0; i<20; i+=2){
        bt.erase(i);
        cout<<bt;
    }
    cout<< bt;
    bt.erase(3);
    cout<<"Deletion done"<<endl;
    cout<< bt;
}
