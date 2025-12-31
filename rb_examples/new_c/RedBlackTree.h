#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <stdio.h>
#include <stdlib.h>

/* Color enum */
typedef enum {
    RED,
    BLACK
} Color;

/* Node structure */
// typedef struct Node {
//     int data;
//     Color color;
//     struct Node* left;
//     struct Node* right;
//     struct Node* parent;
// } Node;

typedef struct RB_Node{
    //!!! TEMP for testing
    void *assoc_c_h_addr;
    //Chunk_Header *assoc_c_h_addr;     //associated Chunk_Header Address
    size_t rb_node_num; //!!! only needed for linked list version
    size_t size;
    struct RB_Node *left;
    struct RB_Node *right;
    struct RB_Node *parent;
    Color color;
}RB_Node;

/* Tree structure */
typedef struct {
    RB_Node* root;
} RedBlackTree;

/* API */
void rbt_init(RedBlackTree* tree);
void rbt_insert(RedBlackTree* tree, int val);
void rbt_insert_node(RedBlackTree* tree, RB_Node *z);
RB_Node *rbt_remove(RedBlackTree* tree, int val);
void rbt_print(const RedBlackTree* tree);
void print_alloc(RB_Node *node);

#endif
