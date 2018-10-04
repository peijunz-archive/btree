#include <iostream>
#include "btree.h"

using namespace std;

int main(){
    btree<int, 4> bt;
    for(int i=0; i<10; i++){
        cout<<bt;
        bt.insert(i);
    }
    cout<< bt <<endl;
}
