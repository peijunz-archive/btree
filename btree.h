#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <cstdint>
#include <algorithm>

using namespace std;

template<typename T, int m>
struct node;

template<typename T, int m>
ostream& operator<< (ostream &os, const node<T, m> * n);

template <typename T, int m>
struct node {
    node* children[m+1] = {nullptr};
    T sep[m-1+1];
    int8_t fill = 0;
    bool is_leaf() const {
        return !children[0];
    }
    bool full() const{
        return fill == m-1;
    }
    bool overflowed() const{
        return fill >= m;
    }
    T* begin(){
        return children;
    }
    T* end(){
        return children + fill;
    }

    void insert_sep(T val, int pos){
        for (int i = fill; i>pos; i--){
            sep[i] = sep[i-1];
        }
        sep[pos] = val;
        fill++;
    }
    void insert_child(node *val, int pos){
        //Must be called after insertion of separator
        for (int i = fill; i>pos; i--){
            children[i] = children[i-1];
        }
        children[pos] = val;
    }

    friend ostream&
    operator<< <>(ostream &os, const node * n);
};

template <typename T, int m>
void recursive_stream (ostream& os, node<T, m> *n){
    if (!n->is_leaf()){
        os << n << ":\t";
        for (int i=0; i < n->fill+1; i++){
            os << n->children[i]<<"\t";
        }
        os << endl;
        for (int i=0; i < n->fill+1; i++){
            recursive_stream(os, n->children[i]);
        }
    }
}

template<typename T, int m>
ostream& operator<< (ostream &os, const node<T, m> * n){
    if (!n->fill){
        os << "()";
    }
    else{
        os << "(";
        for (int i=0; i<n->fill; i++)
            os<<n->sep[i]<<", ";
        os << "\b\b)";
    }
    return os;
}

template <typename T, int m>
struct btree{
    using node_t = node<T, m>;
    node_t *root = nullptr;
    int _size = 0;
    int _depth = 0;
    btree(){
    }
    int size() const{return _size;}
    int depth() const{return _depth;}

    static node_t *
    overflow_leaf(node_t *n){
        node_t * rhs = new node_t;
        copy(n->sep+m-m/2, n->sep+m, rhs->sep);
        n->fill = m - m/2 - 1;
        rhs->fill = m/2;

//        cout<<n<<endl;
//        cout<<rhs<<endl;
        return rhs;
    }

    static node_t *
    overflow_node(node_t *n){
        node_t * rhs = overflow_leaf(n);
        copy(n->children+m-m/2, n->children+m+1, rhs->children);
        return rhs;
    }
    void level_up(node_t *rhs){
//        cout<<">>> Level up"<<endl;
        T root_sep = root->sep[root->fill];
        node_t * old = root;
        root = new node_t;
        root->children[0] = old;
        root->children[1] = rhs;
        root->sep[0] = root_sep;
        root->fill = 1;
        _depth++;
    }
    void level_down(){
//        cout<<">>> Level down"<<endl;
        node_t * old = root;
        root = root->children[0];
        delete old;
        _depth--;
    }
    node_t* insert_from(node_t *n, T val){
        int pos = upper_bound(n->sep, n->sep+n->fill, val) - n->sep;
        if (n->is_leaf()){
//            cout<<"Insert "<<val<<" at "<<pos<<" of "<<n<<endl;
//            cout<<"After insertion: "<<endl;
//            n->print();
            n->insert_sep(val, pos);
            if (n->overflowed()){
//                cout<<"overflow leaves at "<< n->sep[m-m/2-1]<<" for "<<n<<endl;
                return overflow_leaf(n);
            }
//            return nullptr;
        }
        else{
//            cout<<"Insert "<<val<<" at "<<pos<<" of "<<n<<endl;
            node_t * child = n->children[pos];
            node_t * child_new = insert_from(child, val);
            if (child_new){
//                cout<<"Before child insertion"<<n<<endl;
//                cout<<pos<<child->sep[child->fill]<<endl;
                n->insert_sep(child->sep[child->fill], pos);
                n->insert_child(child_new, pos+1);
//                cout<<"After child insertion"<<n<<endl;
                if (n->overflowed()){
//                    cout<<"overflow node at "<< n->sep[m-m/2-1]<<" for "<<n<<endl;
                    return overflow_node(n);
                }
            }
//            return nullptr;
        }
        return nullptr;
    }
    void insert(T val){
        if (!root){
            root = new node_t;
//             cout<<int(root->fill)<<endl;
            root->insert_sep(val, 0);
        }
        else{
            node_t * overflow = insert_from(root, val);
            if (overflow){
                level_up(overflow);
            }
        }
    }
    bool erase_from(node_t *n, T val){
        T* p = lower_bound(n->sep, n->sep+n->fill, val);
        if (*p == val){
            //delete_max()
        }
        else{

        }
        return false;
    }
    void erase(T val){
        if (root){
            bool underflow = erase_from(root, val);
            if (underflow){
                level_down();
            }
        }
    }
    friend ostream& operator<<(ostream& os, btree& bt){
        os <<"BTree: D="<<bt.depth()<< ", m="<<m<< endl;
        if (bt.root){
            if (bt.root->is_leaf()){
                os << bt.root << endl;
                return os;
            }
            recursive_stream(os, bt.root);
        }
        return os;
    }
};

#endif
