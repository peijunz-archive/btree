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
    bool empty() const{
        return fill == 0;
    }
    bool overflowed() const{
        return fill >= m;
    }
    bool underflowed() const{
        return fill < (m-1)/2;
    }
    bool superfluous() const{
        return fill > (m-1)/2;
    }

    void insert_sep(T val, int pos){
        for (int i = fill; i>pos; i--){
            sep[i] = sep[i-1];
        }
        sep[pos] = val;
        fill++;
    }

    void erase_sep(int pos){
        for (int i = pos+1; i<fill; i++){
            sep[i-1] = sep[i];
        }
        fill--;
    }
    void erase_child(int pos){
        // Must be called before erasion of separator
        for (int i = pos+1; i<fill+1; i++){
            children[i-1] = children[i];
        }
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

template<typename T, int m>
void merge_nodes(node<T, m>* lhs, node<T, m>* rhs, T val){
    lhs->sep[lhs->fill] = val;
    copy(rhs->sep, rhs->sep+rhs->fill, lhs->sep+lhs->fill+1);
    copy(rhs->children, rhs->children+rhs->fill+1, lhs->children+lhs->fill+1);
    lhs->fill += rhs->fill + 1;
    delete rhs;
}

template <typename T, int m>
class btree{
public:
    using node_t = node<T, m>;
    int size() const{return _size;}
    int depth() const{return _depth;}

    static node_t *
    overflow_leaf(node_t *n){
        node_t * rhs = new node_t;
        copy(n->sep+m-m/2, n->sep+m, rhs->sep);
        n->fill = m - m/2 - 1;
        rhs->fill = m/2;
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
    void underflow(node_t *n, int pos){
        node_t *self = n->children[pos];
        cout<<"Underflow handling "<<n<<endl;
        if (pos>0 && n->children[pos-1]->superfluous()){
            cout<<"Left rotation "<<endl;
            node_t * sibling = n->children[pos-1];
            // Rotate from left sibling
            self->insert_sep(n->sep[pos-1], 0);
            self->insert_child(sibling->children[sibling->fill], 0);
            n->sep[pos-1] = sibling->sep[sibling->fill-1];
            sibling->erase_child(sibling->fill);
            sibling->erase_sep(sibling->fill-1);
//             sibling->fill--;
        }
        else if(pos < n->fill+1  && n->children[pos+1]->superfluous()){
            // Rotate from right sibling
            cout<<"Right rotation at "<<pos<<endl;
            node_t * sibling = n->children[pos+1];
            self->insert_sep(n->sep[pos], self->fill);
            self->insert_child(sibling->children[0], self->fill);
            n->sep[pos] = sibling->sep[0];
            sibling->erase_child(0);
            sibling->erase_sep(0);
        }
        else{
            // Merge with a sibling
            cout<<"Merging"<<endl;
            bool left_merge = (pos > 0);
            int mid = pos-left_merge, lhs = pos-left_merge, rhs = pos+1-left_merge;
            merge_nodes(n->children[lhs], n->children[rhs], n->sep[mid]);
            n->erase_child(mid+1);
            n->erase_sep(mid);
        }
    }
    void sep_by_max(node_t *n, T *p){
        cout<<"Separate by max from "<<n<<endl;
        if (n->is_leaf()){
            *p = n->sep[n->fill-1];
            cout<<"New separator found: "<<*p<<endl;
            n->fill--;
        }
        else{
            node_t *last_child = n->children[n->fill];
            sep_by_max(last_child, p);
            if (last_child->underflowed()){
                cout<<"Last child underflowed for "<<n<<endl;
                underflow(n, n->fill);
            }
        }
    }
    void erase_from(node_t *n, T val){
        T* p = lower_bound(n->sep, n->sep+n->fill, val);
        int i = p - n->sep;
        cout<<"Erase "<<val<<" from "<<n<<" found "<<i<<endl;
        if (i!=n->fill && *p == val){
            if(n->is_leaf()){
                n->erase_sep(i);
            }
            else{
                sep_by_max(n->children[i], p);
                if (n->children[i]->underflowed()){
                    cout<<"After erasion: child underflowed for "<<n<<" at "<<i<<endl;
                    underflow(n, i);
                }
            }
        }
        else if(!n->is_leaf()){
            erase_from(n->children[i], val);
            if (n->children[i]->underflowed()){
                cout<<"After erasion: child underflowed for "<<n<<" at "<<i<<endl;
                underflow(n, i);
            }
        }
    }
    void erase(T val){
        if (root){
            erase_from(root, val);
            if (root->empty()){
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
private:
    node_t *root = nullptr;
    int _size = 0;
    int _depth = 0;
};

#endif
