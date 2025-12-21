#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include "arena.h"
#include "my_malloc.h"

#define NODE_TABLE_SIZE 8
#define BASE_ARENA_SIZE 65536
#define MAX_SIZE 2048
#define NODE_POOL_SIZE 194   


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
    printf("node->chunks_start_addr: %p\n", node->chunks_start_addr);
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
        printf("RB_NODE[%d] ADDRESS: %p\n", i, &node->arena.node_pool[i]);
        printf("RB_NODE[%d] assoc_c_h_addr (points to chunk header): %p\n", i, node->arena.node_pool[i].assoc_c_h_addr);
        printf("RB_NODE[%d] rb_node_num: %ld\n", i, node->arena.node_pool[i].rb_node_num);
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
    printf("chunks_start_addr: %p\n", node->chunks_start_addr);
        printf("-----------------CHUNK[i]----------------\n");
        void *current_header_addr = node->chunks_start_addr;
        for (uint32_t i = 0; i < rb_node_table[NODE_TABLE_SIZE - 1]; i++)
        {
            Chunk_Header *chunk_header = (Chunk_Header *)current_header_addr;
            printf("Chunk_Header Address: %p\n", current_header_addr);
            printf("Associated RB_Node Address: %p\n", chunk_header->assoc_rb_node);
            printf("Chunk Size (not including chunk header): %ld bytes\n", chunk_header->size);
            if (chunk_header->flags == FREE)
            {
                printf(ANSI_COLOR_CYAN "Chunk Flags: FREE\n" ANSI_COLOR_RESET "\n");
            }
            else if (chunk_header->flags == IN_USE)
            {
                printf(ANSI_COLOR_YELLOW "Chunk Flags: IN_USE\n" ANSI_COLOR_RESET "\n");
            }
            else
            {
                printf(ANSI_COLOR_RED "!!!CHUNK HEADER ERROR!!!\n" ANSI_COLOR_RESET "\n");
            }
            printf("Prev_size(should be same as size): %ld bytes\n", chunk_header->size);

            current_header_addr += sizeof(Chunk_Header) + chunk_header->size;
        }
}

//-------------------ARENA TESTING---------------------------------//
int main(void)
{
    build_rb_idx_table();

    FILE *log = freopen("logs.txt", "w", stdout);
    if(!log){
        perror("freopen");
        return 1;
    }

    //writes the buffer at runtime
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("TABLES:\n");
    //Arena_List_Node *test = my_malloc(62 * sizeof(char));
    Arena_List_Node *test = my_malloc(100 * sizeof(char));
    if (!test)
    {
        printf("Error allocating memory.\n");
    }

    my_malloc(100 * sizeof(char));

    my_malloc(30 * sizeof(char));
    my_malloc(30 * sizeof(char));

    //my_malloc(5000 * sizeof(char));
    //my_malloc(5000 * sizeof(char));

    // for(int i = 0; i < 20; i++){
    //     my_malloc(999 * sizeof(char));
    // }
        //my_malloc(999 * sizeof(char));
    

    printf("test Arena_List_Node starts at: %p\n", test);

    print_tables(test);
    printf("---------------------------------------------------");
    printf("ARENA NODE INFO:\n");
    print_arena_node_info(test);
    
    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // printf("!!!!!!!!!TEST->NEXT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // print_tables(test->next);
    // printf("---------------------------------------------------");
    // printf("ARENA NODE INFO:\n");
    // print_arena_node_info(test->next);

    // printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");

    // printf("Arena 1 Addr: %p\n", test);
    // printf("Arena 2 Addr: %p\n", test->next);


    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // printf("!!!!!!!!!TEST->NEXT->NEXT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // print_tables(test->next->next);
    // printf("---------------------------------------------------");
    // printf("ARENA NODE INFO:\n");
    // print_arena_node_info(test->next->next);

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");

    printf("Arena 1 Addr: %p\n", test);
    printf("Arena 2 Addr: %p\n", test->next);


    fflush(stdout);  
    //restores print to screen
    freopen("/dev/tty", "w", stdout);

    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
    uintptr_t addr3 = 0;
    uintptr_t addr4 = 0;
    //uintptr_t addr5 = 0;
    //uintptr_t addr6 = 0;

    printf("Enter first address: ");
    scanf("%" SCNxPTR, &addr1);
    
    printf("Enter second address: ");
    scanf("%" SCNxPTR, &addr2);
    
    printf("Enter third (32) address: ");
    scanf("%" SCNxPTR, &addr3);
    
    printf("Enter fourth (32) address: ");
    scanf("%" SCNxPTR, &addr4);
    
    // printf("Enter fifth address: ");
    // scanf("%" SCNxPTR, &addr5);
    
    // printf("Enter sixth address: ");
    // scanf("%" SCNxPTR, &addr6);
    
    //char *ptr1 = (char *)addr1 + sizeof(Chunk_Header);
    char *ptr1 = (char *)addr1;
    //memset(ptr1, '!', 5000);
    strcpy(ptr1, "Hello world!\n");

    //char *ptr2 = (char *)addr2 + sizeof(Chunk_Header);
    char *ptr2 = (char *)addr2;
    //memset(ptr2, '+', 5000);
    strcpy(ptr2, "This is Not a Drill!!!\n");
   
    char *ptr3 = (char *)addr3;
    strcpy(ptr3, "NUMERO TRES!\n");
    
    char *ptr4 = (char *)addr4;
    strcpy(ptr4, "NUMERO QUATRO!\n");

    // char *ptr5 = (char *)addr5;
    // memset(ptr5, '!', 5000);

    // char *ptr6 = (char *)addr6;
    // memset(ptr6, '+', 5000);

    printf("ptr1 string: %s", ptr1);
    printf("ptr1 string pointer address: %p\n", ptr1);
    printf("ptr2 string: %s", ptr2);
    printf("ptr2 string pointer address: %p\n", ptr2);
    printf("ptr3 string: %s", ptr3);
    printf("ptr3 string pointer address: %p\n", ptr3);
    printf("ptr4 string: %s", ptr4);
    printf("ptr4 string pointer address: %p\n", ptr4);
    // printf("ptr5 string: %s", ptr5);
    // printf("ptr5 string pointer address: %p\n", ptr5);
    // printf("ptr6 string: %s", ptr6);
    // printf("ptr6 string pointer address: %p\n", ptr6);
 


    my_free(ptr1);

    
    freopen("logs.txt", "a", stdout);
    
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!UPDATE: FREED ptr1!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print_tables(test);
    printf("---------------------------------------------------");
    printf("ARENA NODE INFO:\n");
    print_arena_node_info(test);

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");


    my_free(ptr2);

    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!UPDATE: FREED ptr2!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print_tables(test);
    printf("---------------------------------------------------");
    printf("ARENA NODE INFO:\n");
    print_arena_node_info(test);

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");


    my_free(ptr3);

    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!UPDATE: FREED ptr3!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print_tables(test);
    printf("---------------------------------------------------");
    printf("ARENA NODE INFO:\n");
    print_arena_node_info(test);

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    
    fflush(stdout);  


    my_malloc(100 * sizeof(char));

    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!UPDATE: ADDED space for ptr5!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print_tables(test);
    printf("---------------------------------------------------");
    printf("ARENA NODE INFO:\n");
    print_arena_node_info(test);

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    
    fflush(stdout);  

    //restores print to screen
    freopen("/dev/tty", "w", stdout);

    uintptr_t addr5 = 0;
    printf("Enter fifth (128) address: ");
    scanf("%" SCNxPTR, &addr5);
    
    char *ptr5 = (char *)addr5;
    strcpy(ptr5, "NUMERO CINCO!\n");
    
    printf("ptr5 string: %s", ptr5);
    printf("ptr5 string pointer address: %p\n", ptr5);
    
   
    fflush(stdout);  

    freopen("logs.txt", "a", stdout);


    my_free(ptr4);
    
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!UPDATE: FREED ptr4!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print_tables(test);
    printf("---------------------------------------------------");
    printf("ARENA NODE INFO:\n");
    print_arena_node_info(test);

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    
    fflush(stdout);  



    my_free(ptr5);

    fflush(stdout);
    fclose(log);

    //my_free(ptr5);
    //my_free(ptr6);
   
    printf("ptr2 after free: %s", ptr2);
    printf("ptr2 string pointer address: %p\n", ptr2 - sizeof(Chunk_Header));

    printf("ptr1 string: %s", ptr1);
    printf("ptr1 string pointer address: %p\n", ptr1 - sizeof(Chunk_Header));
  
    // printf("SIZE OF ARENA LIST NODE: %ld", sizeof(Arena_List_Node)); 
    
   
 
    return 0;
}

//--------------MALLOC FRONT END TESTING-----------------------//

// int main(void){
//     char *test1 = my_malloc(62 * sizeof(char));
//     if (!test1)
//     {
//         printf("Error allocating memory.\n");
//     }

//     strcpy(test1, "Hello World!\n");


//     char *test2 = my_malloc(1000 * sizeof(char));
//     if (!test2)
//     {
//         printf("Error allocating memory.\n");
//     }

//     strcpy(test2, "This is Not a Drill!!!\n");
    
    
//     printf("test string: %s", test1);
//     printf("test string pointer address: %p\n", test1);
//     printf("test string: %s", test2);
//     printf("test string pointer address: %p\n", test2);
    
//     my_free(test1);

//     my_free(test2);

//     printf("test string: %s", test1);
//     printf("test string pointer address: %p\n", test1);
//     printf("test string: %s", test2);
//     printf("test string pointer address: %p\n", test2);
 
//     return 0;
// }