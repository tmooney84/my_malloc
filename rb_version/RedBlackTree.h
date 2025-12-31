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
typedef struct RB_Tree{
    RB_Node* root;
} RB_Tree;

/* API */
void rbt_init(RB_Tree* tree);
void rbt_insert(RB_Tree* tree, int val);
void rbt_initial_insert_node(RB_Tree* tree, RB_Node *z);
int rbt_re_insert_node(RB_Tree* tree, RB_Node *z);
RB_Node *rbt_remove(RB_Tree* tree, int val);
void rbt_print(const RB_Tree* tree);
void print_alloc(RB_Node *node);
RB_Node *rbt_find(const RB_Tree *tree, int key);
RB_Node *clear_rb_node(RB_Node *node);
RB_Node *rbt_remove_node(const RB_Tree *tree, RB_Node *node);


#endif
