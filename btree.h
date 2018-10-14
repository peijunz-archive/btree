#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <cstdint>
#include <algorithm>

using namespace std;
/**
 * @brief Node class of btree. It has keys, child nodes and also size of keys
 * @details The degree of a node is actually size + 1.
 *  + If degree > D, it is overflowed
 *  + If 2 * degree < D, it is underflowed
 * overflowed/underflowed nodes must be processed by their parents!
 *
 * @tparam T data type for keys
 * @tparam D Maximum possible degree (i.e. branching factor)
 */
template <typename T, int D>
struct btree_node {
    static_assert(D > 2, "D should be higher than 2");
    static_assert(D < 127, "D should be smaller than 127");

    int8_t size = 0;    ///< Size of keys
    T keys[D-1+1];      ///< Keys as separators
    btree_node* children[D+1] = {nullptr};  ///< Pointers to child nodes

    ~btree_node(){
        if (!is_leaf()){
            for (int i=0; i<size+1; i++){
                delete children[i];
            }
        }
    }
    void reset(){
        children[0] = nullptr;
        size = 0;
    }
    int8_t degree() const{
        return size + 1;
    }
    bool is_leaf() const {
        return !children[0];
    }
    bool full() const{
        return degree() == D;
    }
    bool empty() const{
        return size == 0;
    }
    bool overflowed() const{
        return degree() > D;
    }
    bool underflowed() const{
        return 2 * degree() < D;
    }
    /**
     * @brief superfluous Does the node underflow when its degree is reduced by 1
     */
    bool superfluous() const{
        return 2 * (degree()-1) >= D;
    }

    void insert_key(T val, int pos){
        for (int i = size; i>pos; i--){
            keys[i] = keys[i-1];
        }
        keys[pos] = val;
        size++;
    }
    void insert_child(btree_node *val, int pos){
        //Must be called after insertion of a key
        for (int i = size; i>pos; i--){
            children[i] = children[i-1];
        }
        children[pos] = val;
    }

    void erase_child(int pos){
        // Must be called before erasion of a key
        for (int i = pos+1; i<size+1; i++){
            children[i-1] = children[i];
        }
    }
    void erase_key(int pos){
        for (int i = pos+1; i<size; i++){
            keys[i-1] = keys[i];
        }
        size--;
    }

    /**
     * @brief absorb Absorb keys and children of rhs
     * @param rhs   The node to be absorbed
     * @param pivot The separator between current node and rhs
     */
    void absorb(btree_node* rhs, T pivot){
        keys[size] = pivot;
        copy(rhs->keys, rhs->keys+rhs->size, keys+size+1);
        copy(rhs->children, rhs->children+rhs->size+1, children+size+1);
        size += rhs->size + 1;
        rhs->reset();
        delete rhs;
    }

    friend ostream&
    operator<< (ostream &os, const btree_node * n){
        if (!n->size){
            os << "()";
        }
        else{
            os << "(";
            for (int i=0; i<n->size; i++)
                os<<n->keys[i]<<", ";
            os << "\b\b)";
        }
        return os;
    }
};

template <typename T, int D>
void recursive_stream (ostream& os, btree_node<T, D> *n){
    if (!n->is_leaf()){
        os << n << ":\t";
        for (int i=0; i < n->size+1; i++){
            os << n->children[i]<<"\t";
        }
        os << endl;
        for (int i=0; i < n->size+1; i++){
            recursive_stream(os, n->children[i]);
        }
    }
}


template <typename T, int D>
class btree{
public:
    using node_t = btree_node<T, D>;
    int size() const{return _size;}
    int depth() const{return _depth;}

    ~btree(){
        if (root){
            delete root;
        }
    }
    /**
     * @brief spill_leaf Spill keys of overflowed leaf node
     * @param n overflowed node
     * @return a new node storing right half of the original node
     */
    static node_t *
    spill_leaf(node_t *n){
        node_t * rhs = new node_t;
        copy(n->keys+D-D/2, n->keys+D, rhs->keys);
        n->size = D - D/2 - 1;
        rhs->size = D/2;
        return rhs;
    }
    /**
     * @brief spill_node Spill keys and children of overflowed non-leaf node
     * @param n overflowed node
     * @return new node storing right half of the original node
     */
    static node_t *
    spill_node(node_t *n){
        node_t * rhs = spill_leaf(n);
        copy(n->children+D-D/2, n->children+D+1, rhs->children);
        return rhs;
    }
    /**
     * @brief level_up The level of BTree goes up by one
     * @details This is done after root node overflowed and spilled into rhs
     * @param rhs   Second child of the new root
     */
    void level_up(node_t *rhs){
        T root_sep = root->keys[root->size];
        node_t * old = root;
        root = new node_t;
        root->children[0] = old;
        root->children[1] = rhs;
        root->keys[0] = root_sep;
        root->size = 1;
        _depth++;
    }
    /**
     * @brief level_down The level of BTree goes down by one
     * @details This is done when root node is empty
     */
    void level_down(){
        node_t * old = root;
        root = root->children[0];
        old->reset();
        delete old;
        _depth--;
    }
    /**
     * @brief insert_from Insert value from current node
     * @param n current node
     * @param val
     * @return If current node overflows, return pointer to spilled node
     */
    node_t* insert_from(node_t *n, T val){
        int pos = upper_bound(n->keys, n->keys+n->size, val) - n->keys;
        if (n->is_leaf()){
            n->insert_key(val, pos);
            if (n->overflowed()){
                return spill_leaf(n);
            }
        }
        else{
            node_t * child = n->children[pos];
            node_t * child_new = insert_from(child, val);
            if (child_new){
                n->insert_key(child->keys[child->size], pos);
                n->insert_child(child_new, pos+1);
                if (n->overflowed()){
                    return spill_node(n);
                }
            }
        }
        return nullptr;
    }
    /**
     * @brief insert Insert value into BTree
     * @param val
     */
    void insert(T val){
        if (!root){
            root = new node_t;
            root->insert_key(val, 0);
        }
        else{
            node_t * overflow = insert_from(root, val);
            if (overflow){
                level_up(overflow);
            }
        }
    }
    /**
     * @brief left_rotate Resolve underflow children[pos] by getting child from its left sibling
     * @param n current node
     * @param pos index of child
     * @return Success or not
     */
    bool left_rotate(node_t *n, int pos){
        node_t *self = n->children[pos];
        if (pos>0 && n->children[pos-1]->superfluous()){
            node_t * sibling = n->children[pos-1];
            self->insert_key(n->keys[pos-1], 0);
            self->insert_child(sibling->children[sibling->size], 0);
            n->keys[pos-1] = sibling->keys[sibling->size-1];
            sibling->erase_child(sibling->size);
            sibling->erase_key(sibling->size-1);
            return true;
        }
        return false;
    }

    /**
     * @brief right_rotate Resolve underflow children[pos] by getting child from its right sibling
     * @param n current node
     * @param pos index of child
     * @return Success or not
     */
    bool right_rotate(node_t *n, int pos){
        node_t *self = n->children[pos];
        if(pos < n->size+1  && n->children[pos+1]->superfluous()){
            node_t * sibling = n->children[pos+1];
            self->insert_key(n->keys[pos], self->size);
            self->insert_child(sibling->children[0], self->size);
            n->keys[pos] = sibling->keys[0];
            sibling->erase_child(0);
            sibling->erase_key(0);
            return true;
        }
        return false;
    }
    void merge_children(node_t *n, int pos){
        bool left_merge = (pos > 0);
        int mid = pos-left_merge;
        node_t *lhs = n->children[pos-left_merge], *rhs = n->children[pos+1-left_merge];
        lhs->absorb(rhs, n->keys[mid]);
        n->erase_child(mid+1);
        n->erase_key(mid);
    }
    void rebalance(node_t *n, int i){
        if (!left_rotate(n, i) && !right_rotate(n, i)){
            merge_children(n, i);
        }
    }
    /**
     * @brief set_max_key Set key by max key of current node
     * @param n current node
     * @param key_p Pointer to key that needs to be reset
     */
    void set_max_key(node_t *n, T *key_p){
        if (n->is_leaf()){ ///< Separator found
            *key_p = n->keys[n->size-1];
            n->size--;
        }
        else{   ///< Not found, recursively search
            node_t *last_child = n->children[n->size];
            set_max_key(last_child, key_p);
            if (last_child->underflowed()){
                rebalance(n, n->size);
            }
        }
    }
    /**
     * @brief erase_from Erase from a given node
     * @param n current node
     * @param val value
     */
    void erase_from(node_t *n, T val){
        T* key_p = lower_bound(n->keys, n->keys+n->size, val);
        int i = key_p - n->keys;
        if (i!=n->size && *key_p == val){///< Found val at current node
            if(n->is_leaf()){   ///< For leaf node, simply delete
                n->erase_key(i);
            }
            else{   ///< For non-leaf, set new key to be max of left child
                set_max_key(n->children[i], key_p);
                if (n->children[i]->underflowed()){
                    rebalance(n, i);
                }
            }
        }
        else if(!n->is_leaf()){ ///< Not found, search recursively
            erase_from(n->children[i], val);
            if (n->children[i]->underflowed()){
                rebalance(n, i);
            }
        }
    }
    /**
     * @brief erase Erase value from the btree
     * @param val value
     */
    void erase(T val){
        if (root){
            erase_from(root, val);
            if (root->empty()){
                level_down();
            }
        }
    }
    friend ostream& operator<<(ostream& os, btree& bt){
        os <<"BTree: D="<<bt.depth()<< ", m="<<D<< endl;
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
