#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "arena.h"
#include "my_malloc.h"

#define NODE_TABLE_SIZE 8
#define BASE_ARENA_SIZE 65536
#define MAX_SIZE 3072

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"

// value 32 64 128 256 512 1k 2k total
// 64kb: 64 40 30  24  16  12  8 194
// used >>> when total zero if free() then remove arena
// static const uint32_t rb_node_table[NODE_TABLE_SIZE] = {64, 40, 30, 24, 16, 12, 8, 194};
// static uint32_t rb_idx_table[NODE_TABLE_SIZE] = {};
// uint32_t rb_node_used[NODE_TABLE_SIZE];

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

static void *arena_list_start = NULL;

void print_tables(Arena_List_Node *node)
{
    printf("rb_node_table: ");
    for (int i = 0; i < NODE_TABLE_SIZE; i++)
    {
        printf("%d ", rb_node_table[i]);
    }
    printf("\n");

    printf("rb_idx_table: ");
    for (int i = 0; i < NODE_TABLE_SIZE; i++)
    {
        printf("%d ", rb_idx_table[i]);
    }
    printf("\n");

    printf("rb_node_used: ");
    for (int i = 0; i < NODE_TABLE_SIZE; i++)
    {
        printf("%d ", node->arena.arena_header.node_table.rb_node_used[i]);
    }
    printf("\n");

    return;
}

void print_arena_node_info(Arena_List_Node *node)
{
    printf("Arena_List_Node *node addr: %p\n", node); 
    printf("node->next: %p\n", node->next);
    printf("node->prev: %p\n", node->prev);
    printf("node->arena: %p\n", (void *)&node->arena);
    printf("------------------ARENA HEADER----------------------\n");
    printf("node->arena.arena_header.base: %p\n", node->arena.arena_header.base);
    printf("node->arena.arena_header.large_alloc: %d\n", node->arena.arena_header.large_alloc);
    printf("node->arena.arena_header.size: %ld bytes\n", node->arena.arena_header.size);
    printf("node->arena.arena_header.free_tree.root: %p\n", node->arena.arena_header.free_tree.root);
    printf("node->arena.arena_header.rb_node_pool: %p\n", node->arena.arena_header.rb_node_pool);
    printf("node->arena.arena_header.chunks_start_addr: %p\n", node->arena.arena_header.chunks_start_addr);
    printf("node->arena.arena_header.node_table.rb_node_used: ");
    for (int i = 0; i < NODE_TABLE_SIZE; i++)
    {
        printf("%d ", node->arena.arena_header.node_table.rb_node_used[i]);
    }
    printf("\n");
    printf("------------------RB TREE----------------------\n");
    printf("node->arena.arena_header.free_tree.root: %p\n", node->arena.arena_header.free_tree.root);
    printf("...PUT TREE PRINT FUNCTION HERE...\n");
    printf("------------------RB POOL----------------------\n");
    printf("------------------RB POOL----------------------\n");
    printf("RB POOL starts at address: %p\n", &node->arena.node_pool);
    for (int i = 0; i < NODE_POOL_SIZE; i++)
    {
        printf("------------------RB NODE[%d]----------------------\n", i);
        printf("RB_NODE[%d] assoc_c_h_addr (points to chunk header): %p\n", i, node->arena.node_pool[i].assoc_c_h_addr);
        printf("RB_NODE[%d] size: %ld\n", i, node->arena.node_pool[i].size);
        printf("RB_NODE[%d] left: %p\n", i, node->arena.node_pool[i].left);
        printf("RB_NODE[%d] right: %p\n", i, node->arena.node_pool[i].right);
        printf("RB_NODE[%d] parent: %p\n", i, node->arena.node_pool[i].parent);

        if (node->arena.node_pool[i].color == NO_COLOR)
        {
            printf(ANSI_COLOR_YELLOW "RB_NODE[%d] enum COLOR: NO_COLOR" ANSI_COLOR_RESET "\n", i);
        }
        else if (node->arena.node_pool[i].color == RED)
        {
            printf(ANSI_COLOR_RED "RB_NODE[%d] enum COLOR: RED" ANSI_COLOR_RESET "\n", i);
        }
        else if (node->arena.node_pool[i].color == BLACK)
        {
            printf(ANSI_COLOR_GREEN "RB_NODE[%d] enum COLOR: RED" ANSI_COLOR_RESET "\n", i);
        }
        else
        {
            perror("RB_NODE: Unable to get node color\n");
        }
    }
    printf(ANSI_COLOR_MAGENTA "------------------CHUNKS USER ARENA----------------------\n" ANSI_COLOR_RESET "\n");
    // printf("chunks_start_addr: %p\n", node->arena.arena_header.chunks_start_addr);
    // for (int i = 0; i < NODE_POOL_SIZE; i++)
    // {
    //     printf("-----------------CHUNK[i]----------------\n");
    //     void *current_header_addr = node->arena.arena_header.chunks_start_addr;
    //     for (int i = 0; i < rb_node_table[NODE_TABLE_SIZE - 1]; i++)
    //     {
    //         Chunk_Header *chunk_header = (Chunk_Header *)current_header_addr;
    //         printf("Chunk_Header Address: %p\n", current_header_addr);
    //         printf("Chunk Size (not including chunk header): %ld bytes", (chunk_header->size - sizeof(Chunk_Header)));
    //         if (chunk_header->flags == FREE)
    //         {
    //             printf(ANSI_COLOR_CYAN "Chunk Flags: FREE\n" ANSI_COLOR_RESET "\n");
    //         }
    //         else if (chunk_header->flags == IN_USE)
    //         {
    //             printf(ANSI_COLOR_YELLOW "Chunk Flags: IN_USE\n" ANSI_COLOR_RESET "\n");
    //         }
    //         else
    //         {
    //             printf(ANSI_COLOR_RED "!!!CHUNK HEADER ERROR!!!\n" ANSI_COLOR_RESET "\n");
    //         }
    //         printf("Prev_size(should be same as size): %ld bytes", chunk_header->size - sizeof(Chunk_Header));

    //         current_header_addr += sizeof(Chunk_Header) + chunk_header->size;
    //     }
    // }
}

int main(void)
{
    // char *test = my_malloc(20 * sizeof(char));
    // if (!test)
    // {
    //     printf("Error allocating memory.\n");
    // }

    // printf("test string: %s", test);
    // printf("test string pointer address: %p", test);
build_rb_idx_table();


arena_list_start = NULL;
    printf("TABLES:\n");
    Arena_List_Node *a_node = malloc(sizeof(a_node));
    if (!a_node)
    {
        perror("malloc error.\n");
        return -1;
    }


    print_tables(a_node);
    printf("---------------------------------------------------");
    printf("ARENA NODE INFO:\n");
    print_arena_node_info(a_node);
    return 0;
}
