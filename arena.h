#include <stdint.h>
#include <stdio.h>

#define NODE_TABLE_SIZE 9

typedef struct RB_Node{
    void *addr;
    size_t size;
    struct RBNode *left;
    struct RBNode *right;
    struct RBNode *parent;
    enum{RED, BLACK} color;
}RB_Node;

typedef struct RB_Tree{
    RB_Node *root;
} RB_Tree;

typedef struct Node_Table {
        //value 3 2 1 512 256 128 64 32 Total
        //count
        //used >>> when total zero if free() then remove arena
        //free
        uint32_t rb_node_value[NODE_TABLE_SIZE]; //not sure if I need
        uint32_t rb_node_count[NODE_TABLE_SIZE];
        uint32_t rb_node_used[NODE_TABLE_SIZE];
    }Node_Table;

typedef struct Arena_Header{
    void *base;
    size_t size;
    RB_Tree free_tree;
    RB_Node *rb_node_pool;
    Node_Table node_table;
    //pthread_mutex_t lock; if wanted multithread-safe
    //size_t used_bytes
    struct Arena_Header *next;
    struct Arena_Header *prev;
}Arena_Header;

typedef struct Chunk_Header{
    size_t size; //full chunk size including header
    uint32_t flags; // FREE or IN_USE
}Chunk_Header;


//=====================================================//
// RB_Node Pool
//  [RB_Node 0] [RB_Node 1] [RB_Node 2] ...
//=====================================================//

//=====================================================//
//  Chunk headers and Payloads 
//  user memory/payload returned by malloc()    
//
//=====================================================//




/*
======================== ARENA =========================
base = 0x10000000
size = 64 KB
page_bitmap → [ bytes ]
free_tree.root → &RBNode_A
rbnode_pool → &RBNode_0

-------------------- RBNode Pool -----------------------
RBNode_0 (A):
    addr  → 0x10003000  (ChunkHeader of 4 KB chunk)
    size  = 4096
    left  = &RBNode_1
    right = &RBNode_2
    parent= NULL
    color = BLACK

RBNode_1 (B):
    addr  → 0x10001000  (ChunkHeader of 8 KB chunk)
    size  = 8192
    left  = NULL
    right = NULL
    parent= &RBNode_0
    color = RED

RBNode_2 (C):
    addr  → 0x10008000  (ChunkHeader of 12 KB chunk)
    size  = 12288
    left  = NULL
    right = NULL
    parent= &RBNode_0
    color = RED

------------------- USER AREA --------------------------
0x10001000: [ChunkHeader][ 8 KB free chunk      ]
0x10003000: [ChunkHeader][ 4 KB free chunk      ]   >>> these could be allocated as well
0x10008000: [ChunkHeader][12 KB free chunk      ]
========================================================

*/