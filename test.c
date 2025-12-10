#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define NODE_TABLE_SIZE 8
#define BASE_ARENA_SIZE 65536
#define MAX_SIZE 3072 
#define NODE_POOL_SIZE 194   

static const uint32_t rb_node_size[NODE_TABLE_SIZE] = {32, 64, 128, 256, 512, 1024, 2048, 0};
static const uint32_t rb_node_table[NODE_TABLE_SIZE] = {64, 40, 30, 24, 16, 12, 8, 194};

        //idxTB: 0 63 103 133 157 173 185 
static uint32_t rb_idx_table[NODE_TABLE_SIZE] = {}; //??? IS THIS NEEDED???

int build_rb_idx_table(void)
{
    int idx = -1;
    rb_idx_table[0] = 0;
    for (int i = 0; i < NODE_TABLE_SIZE - 1; i++)
    {
        idx += rb_node_table[i];
        rb_idx_table[i + 1] = idx;
    }

    // should give 193 as the last index
    rb_idx_table[NODE_TABLE_SIZE - 1] = rb_node_table[NODE_TABLE_SIZE - 1] - 1;

    return 0;
}

size_t get_rb_node_size(uint32_t idx)
{
    for (uint32_t i = 0; i < NODE_TABLE_SIZE - 1; i++)
    {
        if (idx >= rb_idx_table[i] && idx <= rb_idx_table[i + 1] && i < NODE_TABLE_SIZE - 1)
        {
            return rb_node_size[i];
        }
    }
    perror("Error getting rb_node_size\n");
    return -1;
}

int main(void){
        rb_idx_table[NODE_POOL_SIZE] = -1;
        memset(rb_idx_table, 0, NODE_TABLE_SIZE + 1);

        printf("before: rb_idx_table:  ");
    for(int i = 0; i < NODE_TABLE_SIZE; i++){
        printf("%d ", rb_idx_table[i]);
    }
    printf("\n");

    build_rb_idx_table();

        printf("rb_node_table: ");
    for(int i = 0; i < NODE_TABLE_SIZE; i++){
        printf("%d ", rb_node_table[i]);
    }

    printf("\n");

        printf("rb_idx_table:  ");
    for(int i = 0; i < NODE_TABLE_SIZE; i++){
        printf("%d ", rb_idx_table[i]);
    }

    printf("\n");
    printf("--------------------------------------------\n");
    
    printf("chunk_size: %ld", get_rb_node_size(177));



    // char *name = malloc(sizeof(char) * 15);

    // char tester[15] = "HEY THERE";
    // strcpy(name, "Hey There");

    // printf("Malloc: %s\n", name);
    // printf("Memory: %p\n", name);
    // free(name);

    // printf("TESTER: %s\n", tester);
    // printf("Freed: %s\n", name);
    // printf("Memory: %p\n", name);
}