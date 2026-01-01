#ifndef ARENA_H 
#define ARENA_H 

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define NODE_TABLE_SIZE 8
#define BASE_ARENA_SIZE 65536
#define MAX_SIZE 2048 
#define NODE_POOL_SIZE 194   


//NEEDS TO BE MANUALLY UPDATED TO LAST ELEMENT IN rb_node_table
        //value 32 64 128 256 512 1k 2k total
        //64kb: 64 40 30  24  16  12  8 194
        //used >>> when total zero if free() then remove arena
static const uint32_t rb_node_size[NODE_TABLE_SIZE] = {32, 64, 128, 256, 512, 1024, 2048, 0};
static const uint32_t rb_node_table[NODE_TABLE_SIZE] = {64, 40, 30, 24, 16, 12, 8, 194};

        //idxTB: 0 63 103 133 157 173 185 

extern uint32_t rb_idx_table[NODE_TABLE_SIZE];

typedef enum {
    NO_COLOR = 0,
    RED      = 1,
    BLACK    = 2
} Color;

typedef enum{
    ADD_TO_TABLE = 0,
    DELETE_FROM_TABLE = 1,
}Chunk_Op;

typedef struct __attribute__((aligned(16))) Chunk_Header{
    void *assoc_rb_node;
    size_t size; //full chunk size including header
    enum{FREE, IN_USE, NA} flags; // FREE or IN_USE
    size_t prev_size;
}Chunk_Header;

typedef struct RB_Node{
    Chunk_Header *assoc_c_h_addr;     //associated Chunk_Header Address
    size_t rb_node_num; //!!! only needed for linked list version
    size_t size;
    struct RB_Node *left;
    struct RB_Node *right;
    struct RB_Node *parent;
    Color color;
}RB_Node;

typedef struct RB_Tree{
    RB_Node *root;
} RB_Tree;

typedef struct Node_Table {
        uint32_t rb_node_used[NODE_TABLE_SIZE];
    }Node_Table;

typedef struct __attribute__((aligned(16))) Arena_Header{
    void *base;
    bool large_alloc; 
    size_t size;
    RB_Tree free_tree;
    RB_Node *rb_node_pool;
    //void *chunks_start_addr;
    Node_Table node_table;
    //pthread_mutex_t lock; if wanted multithread-safe
    //size_t used_bytes
}Arena_Header;

typedef struct __attribute__((aligned(16))) Arena{
    Arena_Header arena_header;
    RB_Node node_pool[NODE_POOL_SIZE];
}Arena;

typedef struct __attribute__((aligned(16))) Arena_List_Node{
    struct Arena_List_Node *next;
    struct Arena_List_Node *prev;
    Arena arena;
    char *chunks_start_addr;
    //END of Arena_List_Node
    //---------------------------
    //USER SPACE CHUNKS START AFTER
    //void *chunks_start_addr;
    //Chunks_Header
    //***USER MALLOC ***//
}Arena_List_Node;

#endif