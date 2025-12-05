#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define NODE_TABLE_SIZE 8
#define BASE_ARENA_SIZE 65536
#define MAX_SIZE 3072 
#define NODE_POOL_SIZE 194   

//NEEDS TO BE MANUALLY UPDATED TO LAST ELEMENT IN rb_node_table
        //value 32 64 128 256 512 1k 2k total
        //64kb: 64 40 30  24  16  12  8 194
        //used >>> when total zero if free() then remove arena
static const uint32_t rb_node_table[NODE_TABLE_SIZE] = {64, 40, 30, 24, 16, 12, 8, 194};
static uint32_t rb_idx_table[NODE_TABLE_SIZE] = {};

typedef enum {
    NO_COLOR = 0,
    RED      = 1,
    BLACK    = 2
} Color;

typedef enum{
    ADD_TO_TABLE = 0,
    DELETE_FROM_TABLE = 1,
}Chunk_Op;

static const char *ColorNames[] = {
    "NO_COLOR",
    "RED",
    "BLACK",
};

typedef struct RB_Node{
    void *addr;
    size_t size;
    struct RBNode *left;
    struct RBNode *right;
    struct RBNode *parent;
    Color color;
}RB_Node;

typedef struct RB_Tree{
    RB_Node *root;
} RB_Tree;

typedef struct Node_Table {
        uint32_t rb_node_used[NODE_TABLE_SIZE];
    }Node_Table;

typedef struct Arena_Header{
    void *base;
    bool large_alloc; 
    size_t size;
    RB_Tree free_tree;
    RB_Node *rb_node_pool;
    void *chunks_start_addr;
    Node_Table node_table;
    //pthread_mutex_t lock; if wanted multithread-safe
    //size_t used_bytes
}Arena_Header;

typedef struct Chunk_Header{
    size_t size; //full chunk size including header
    enum{FREE, IN_USE} flags; // FREE or IN_USE
    size_t prev_size;
}Chunk_Header;

//!!! is this necessary?
typedef struct Chunk{
    Chunk_Header chunk_header;
    //data stored after header;
}Chunk;

typedef struct Arena{
    Arena_Header arena_header;
    RB_Node node_pool[NODE_POOL_SIZE];
    //---------------------------
    //USER SPACE CHUNKS START AFTER
    //void *chunks_start_addr;
}Arena;

typedef struct Arena_List_Node{
    //next && prev node above so chunks don't overwrite the pointers
    struct Arena_List_Node *next;
    struct Arena_List_Node *prev;
    Arena arena;
}Arena_List_Node;

//header_size = sizeof(Chunk_Header);
//Next Chunk stored &chunk[0] + header_size + chunk.chunk_header.size;
//continues through the rest of the user area


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