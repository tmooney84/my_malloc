#ifndef MY_MALLOC_H 
#define MY_MALLOC_H 

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "arena.h"
#include "bitwise_helpers.h"

int build_rb_idx_table();
size_t get_rb_node_size(uint32_t idx);
void set_default_arena_header(Arena_List_Node *node);
void build_default_rb_node_pool(Arena_List_Node *node);
void build_default_chunks_area(Arena_List_Node *node);
RB_Node *build_free_tree(Arena_List_Node *al_node);
void update_table(Arena_List_Node *node, size_t chunk_size, Chunk_Op op);
void update_table_with_idx(Arena_List_Node *node, size_t chunk_size_idx, Chunk_Op op);
char *alloc_chunk_size(Arena_List_Node *node, size_t chunk_size_idx);
char *alloc_arena_chunk(size_t m_size, Arena_List_Node *node);
char *large_allocation(size_t m_size, Arena_List_Node *node);
void build_arena(Arena_List_Node *node);
Arena_List_Node *create_default_arena_list_node();
Arena_List_Node *create_custom_arena_list_node(size_t size);
void *my_malloc(size_t m_size);
//Arena_List_Node *my_malloc(size_t m_size);

bool check_zero_used(Arena_List_Node *node);
void unmap_arena_list_node(Arena_List_Node *curr_node);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

#endif