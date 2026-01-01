#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stddef.h>

#include "arena.h"
#include "bitwise_helpers.h"

static void *arena_list_start = NULL;

uint32_t rb_idx_table[NODE_TABLE_SIZE] = {0};

void print_hex(const void *data, size_t len) {
    const unsigned char *p = data;

    for (size_t i = 0; i < len; i++) {
        printf("%02X ", p[i]);
    }
    putchar('\n');
    return;
}

int build_rb_idx_table()
{
    int idx = -1;
    rb_idx_table[0] = 0;
    for (int i = 0; i < NODE_TABLE_SIZE - 2; i++)
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

void set_default_arena_header(Arena_List_Node *node)
{
    node->arena.arena_header.base = &node->arena;
    node->arena.arena_header.large_alloc = false;
    node->arena.arena_header.size = BASE_ARENA_SIZE;
    node->arena.arena_header.free_tree.root = NULL;
    node->arena.arena_header.rb_node_pool = node->arena.node_pool;
    memset(node->arena.arena_header.node_table.rb_node_used, 0, NODE_TABLE_SIZE * sizeof(uint32_t));
    return;
}

void build_default_rb_node_pool(Arena_List_Node *node)
{
    RB_Node *pool = node->arena.node_pool;

    for (uint32_t i = 0; i < rb_node_table[NODE_TABLE_SIZE - 1]; i++)
    {
        // pool[i].addr >>> set in build_default_chunks_area() function
        pool[i].rb_node_num = i;
        pool[i].size = get_rb_node_size(i);
        pool[i].left = NULL;
        pool[i].right = NULL;
        pool[i].parent = NULL;
        pool[i].color = NO_COLOR;
    }

    return;
}

void build_default_chunks_area(Arena_List_Node *node)
{
    char *current_header_addr = node->chunks_start_addr;
    size_t chunk_count = 0;

    for (; chunk_count < rb_node_table[NODE_TABLE_SIZE - 1]; chunk_count++)
    {
        Chunk_Header *chunk_header = (Chunk_Header *)current_header_addr;
        node->arena.node_pool[chunk_count].assoc_c_h_addr = chunk_header; // sets pool[i].addr that points to chunk header
        chunk_header->assoc_rb_node = &node->arena.node_pool[chunk_count];
        chunk_header->size = get_rb_node_size(chunk_count);
        chunk_header->flags = FREE;
        chunk_header->prev_size = chunk_header->size; // future use in coalescing

        current_header_addr += sizeof(Chunk_Header) + chunk_header->size;
    }

    return;
}

//==========================START OF RB TREE /LINKED LIST OPERATIONS=====================//
RB_Node *build_free_tree(Arena_List_Node *al_node)
{
    RB_Node *head = &al_node->arena.node_pool[0];
    RB_Node *itr = head;
    for (int i = 0; i < NODE_POOL_SIZE - 1; i++)
    {
        // set right
        itr[i].right = &itr[i + 1];
        // set parent of child
        itr[i + 1].parent = &itr[i];
    }

    return head;
}

void update_table(Arena_List_Node *node, size_t chunk_size, Chunk_Op op)
{
    size_t cs_idx = chunk_size_index(chunk_size);

    if (op == DELETE_FROM_TABLE)
    {
        node->arena.arena_header.node_table.rb_node_used[cs_idx]--;
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]--;
    }
    else if (op == ADD_TO_TABLE)
    {
        node->arena.arena_header.node_table.rb_node_used[cs_idx]++;
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]++;
    }

    return;
}

void update_table_with_idx(Arena_List_Node *node, size_t chunk_size_idx, Chunk_Op op)
{
    if (op == DELETE_FROM_TABLE)
    {
        node->arena.arena_header.node_table.rb_node_used[chunk_size_idx]--;
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]--;
    }
    else if (op == ADD_TO_TABLE)
    {
        node->arena.arena_header.node_table.rb_node_used[chunk_size_idx]++;
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]++;
    }

    return;
}

char *alloc_chunk_size(Arena_List_Node *node, size_t chunk_size_idx)
{
    char *my_malloc_ptr = NULL;

    size_t chunk_size = rb_node_size[chunk_size_idx];
    size_t num_possible_chunks = 0;

    for (size_t i = chunk_size_idx; i < NODE_TABLE_SIZE - 1; i++)
    {
        num_possible_chunks += rb_node_table[i];
    }

    size_t start_idx = node->arena.arena_header.node_table.rb_node_used[chunk_size_idx];

    RB_Node *node_pool = node->arena.node_pool;

    for (uint32_t i = start_idx; i < rb_node_table[NODE_TABLE_SIZE - 1] - 1; i++)
    {
        if (node_pool[i].size >= chunk_size && (node_pool[i].parent != NULL || node_pool[i].right != NULL))
        {
            Chunk_Header *ch = node_pool[i].assoc_c_h_addr;

            // if chunk found... set RB_Node addr, update table
            if (ch->flags == FREE)
            {
                my_malloc_ptr = (char *)ch + sizeof(Chunk_Header);

                // Chunk_Header
                ch->flags = IN_USE;

                // remove node from ll
                RB_Node *prev = node_pool[i].parent;

                // RB_TREE >>> allocating first node... need to reset head
                // if first, reset free_tree.root
                if (prev == NULL)
                {
                    node->arena.arena_header.free_tree.root = node_pool[i].right;
                    node_pool[i].right->parent = NULL;
                }

                // if in list and not last entry of node_pool for current arena
                else if (node_pool[i].right != NULL)
                {
                    prev->right = node_pool[i].right;
                    node_pool[i].right->parent = prev;
                }

                else
                {
                    prev->right = NULL;
                }

                // RB_NODE null out right (and left) to more easily track
                node_pool[i].left = NULL;
                node_pool[i].right = NULL;
                node_pool[i].parent = NULL;
                // change color in RB Version

                // check chunk size index for table update
                int32_t used_chunk_size_idx = chunk_size_index(ch->size);

                // TABLE UPDATE
                update_table_with_idx(node, used_chunk_size_idx, ADD_TO_TABLE);

                break;
            }
        }
    }
    return my_malloc_ptr;
}

char *alloc_arena_chunk(size_t m_size, Arena_List_Node *node)
{
    char *my_malloc_ptr = NULL;

    // find chunk size and starting_size_index
    int32_t cs_idx = chunk_size_index(m_size); // rb_node_table[cs_idx] = chunk size
    uint32_t *used_table = &node->arena.arena_header.node_table.rb_node_used[0];

    // find open chunk size
    for (size_t i = cs_idx; i < NODE_POOL_SIZE - 1; i++)
    {
        // chunk_size not full
        if (used_table[i] != rb_node_table[i])
        {
            // for RB Tree... find free node of that size
            my_malloc_ptr = alloc_chunk_size(node, i);
            return my_malloc_ptr;
        }
        // if chunk_size full check next node size group
    }

    return my_malloc_ptr;
}

char *large_allocation(size_t m_size, Arena_List_Node *node)
{
    char *my_malloc_ptr = NULL;

    Chunk_Header *chunk_header = (Chunk_Header *)node->chunks_start_addr;
    chunk_header->flags = IN_USE;
    chunk_header->size = m_size;
    chunk_header->prev_size = chunk_header->size;
    my_malloc_ptr = (char *)chunk_header + sizeof(Chunk_Header);

    return my_malloc_ptr;
}
//==========================END OF RB TREE /LINKED LIST OPERATIONS=====================//

void build_arena(Arena_List_Node *node)
{
    node->chunks_start_addr = (char *)node + sizeof(Arena_List_Node);

    set_default_arena_header(node);
    build_default_rb_node_pool(node);
    build_default_chunks_area(node);

    node->arena.arena_header.free_tree.root = build_free_tree(node);

    return;
}

Arena_List_Node *create_default_arena_list_node()
{
    Arena_List_Node *node = NULL;

    void *mmap_region = mmap(0, BASE_ARENA_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    node = (Arena_List_Node *)mmap_region;
    build_arena(node);

    node->next = NULL;
    node->prev = NULL;
    return node;
}

Arena_List_Node *create_custom_arena_list_node(size_t size)
{
    Arena_List_Node *node = NULL;

    size_t m_size = (size + sizeof(Chunk_Header) + 1 * sizeof(Arena_List_Node) + 5000);

    void *mmap_region = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    node = (Arena_List_Node *)mmap_region;
    node->next = NULL;
    node->prev = NULL;
    node->arena.arena_header.base = &node->arena;
    node->arena.arena_header.large_alloc = true;
    node->arena.arena_header.size = m_size;
    node->arena.arena_header.rb_node_pool = node->arena.node_pool;
    node->chunks_start_addr = (char *)node + sizeof(Arena_List_Node);

    // only one node/chunk is used
    node->arena.node_pool[0].assoc_c_h_addr = (Chunk_Header *)node->chunks_start_addr;
    node->arena.node_pool[0].rb_node_num = 0;
    node->arena.node_pool[0].size = size;
    node->arena.node_pool[0].left = NULL;
    node->arena.node_pool[0].right = NULL;
    node->arena.node_pool[0].parent = NULL;
    node->arena.node_pool[0].color = NO_COLOR;

    Chunk_Header *header = (Chunk_Header *)node->chunks_start_addr;
    header->assoc_rb_node = (void *)&node->arena.node_pool[0];
    header->flags = NA;
    header->size = 0;
    header->prev_size = 0;

    return node;
}

void *my_malloc(size_t m_size)
//!!!Arena_List_Node *my_malloc(size_t m_size)
{
    void *my_malloc_ptr = NULL;

    if (0 != build_rb_idx_table())
    {
        perror("Unable to build rb_idx_table");
        return NULL;
    }

    //***ARENAS linked list will be 64kb

    // 0) m_size 0 does what???
    if (m_size == 0)
    {
        // return my_malloc_ptr
        return NULL;
    }

    // 1) NO ARENA, CREATE/MMAP ARENA

    // 1.1) NO ARENA AND SMALL
    if (!arena_list_start && m_size <= MAX_SIZE)
    {
        Arena_List_Node *head = create_default_arena_list_node();
        arena_list_start = (void *)head;
        // get initial chunk and update rb_tree + table accordingly
        my_malloc_ptr = (void *)alloc_arena_chunk(m_size, head);
        if (my_malloc_ptr == NULL)
        {
            perror("Unable to make small allocation with newly created arena");
            return NULL;
        }

        return my_malloc_ptr;
        //!!!return head;
    }

    // 1.2) NO ARENA AND BIG
    else if (!arena_list_start && m_size > MAX_SIZE)
    {
        Arena_List_Node *head = create_custom_arena_list_node(m_size);
        arena_list_start = (void *)head;

        my_malloc_ptr = (void *)large_allocation(m_size, head);

        if (my_malloc_ptr == NULL)
        {
            perror("Unable to make small allocation with newly created arena");
            return NULL;
        }

        return my_malloc_ptr;
        ///!!return head;
    }

    // 2) ARENA LIST EXISTS

    // 2.1 ARENA LIST EXISTS and SMALL
    if (arena_list_start && m_size <= MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;

        // iterate through arena list
        while (itr != NULL)
        {
            // if LARGE SIZE custom mmap alloc... skip to next arena
            if (itr->arena.arena_header.large_alloc == true && itr->next == NULL)
            {
                break;
            }

            else if (itr->arena.arena_header.large_alloc == true && itr->next != NULL)
            {
                itr = itr->next;
            }

            else if (itr->arena.arena_header.large_alloc == false)
            {
                my_malloc_ptr = (void *)alloc_arena_chunk(m_size, itr);
                if (my_malloc_ptr != NULL)
                {
                    return my_malloc_ptr;
                }
                if(my_malloc_ptr == NULL && itr->next)
                {
                    itr = itr->next;
                }
                else if(my_malloc_ptr == NULL && itr->next == NULL){
                    break;
                }
            }
            // if chunk not found, go to next arena
        }

        // if not found in arena linked list
        Arena_List_Node *new_node = create_default_arena_list_node();
        if (!new_node)
        {
            perror("Unable to create new node.\n");
            return NULL;
        }

        // add to arena list
        itr->next = new_node;
        new_node->prev = itr;
        my_malloc_ptr = alloc_arena_chunk(m_size, itr->next);

        printf("POINTER ADDR arena and small @@@@@@@@@@@@@@@@@@@@@ %p\n", my_malloc_ptr);
        return my_malloc_ptr;
        //!!!return itr->next;
    }

    // 2.2 ARENA LIST EXISTS and LARGE
    else if (arena_list_start && m_size > MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
        Arena_List_Node *node = create_custom_arena_list_node(m_size);
        my_malloc_ptr = large_allocation(m_size, node);

        for (; itr->next != NULL; itr = itr->next)
            ;

        // add to arena list
        itr->next = node;
        node->prev = itr;

        printf("POINTER ADDR arena exists and big @@@@@@@@@@@@@@@@@@@@@ %p\n", my_malloc_ptr);
        return my_malloc_ptr;
        //!!!return node;
    }

    ///!!!return my_malloc_ptr;

    return NULL; 
}

bool check_zero_used(Arena_List_Node *node)
{
    return node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1] == 0;
}

void unmap_arena_list_node(Arena_List_Node *curr_node)
{
    // remove from arena_list
    Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
    size_t m_size = curr_node->arena.arena_header.size;

    if (itr == curr_node && itr->next == NULL)
    {
        arena_list_start = NULL;

        if (0 == munmap((void *)curr_node, m_size))
        {
            return;
        }
        else
        {
            perror("Unable to unmap arena from memory\n");
            return;
        }
    }
    // first node and next node
    else if (itr == curr_node && itr->next)
    {
        Arena_List_Node *next = itr->next;
        next->prev = NULL;
        arena_list_start = (void *)itr->next;

        if (0 == munmap((void *)curr_node, m_size))
        {
            return;
        }
        else
        {
            perror("Unable to unmap arena from memory\n");
            return;
        }
    }
    while (itr != curr_node && itr != NULL)
    {
        itr = itr->next;
    }

    // in middle
    if (itr == curr_node && itr->next != NULL)
    {
        Arena_List_Node *prev = itr->prev;
        Arena_List_Node *next = itr->next;
        prev->next = itr->next;
        next->prev = itr->prev;
        return;
    }
    else if (itr == curr_node && itr->next == NULL)
    {
        itr->prev->next = NULL;
    }
    else
    {
        perror("Unable to find arena for removal");
        return;
    }

    if (0 == munmap((void *)curr_node, m_size))
    {
        return;
    }
    perror("Unable to unmap arena from memory\n");
    return;
}

void my_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    // update Chunk_Header
    Chunk_Header *curr_head = (Chunk_Header *)(ptr - sizeof(Chunk_Header));
    curr_head->flags = FREE;

    // zero out allocated space
    memset((char *)curr_head + sizeof(Chunk_Header), 0, curr_head->size);

    // update RB_Node and add back in list + table
    RB_Node *curr_rb_node = (RB_Node *)curr_head->assoc_rb_node;
    size_t freed_node_num = curr_rb_node->rb_node_num;
    size_t freed_node_size = curr_rb_node->size;

    char *rb_ptr = (char *)curr_rb_node;
    size_t curr_rb_node_offset = sizeof(RB_Node) * freed_node_num;

    char *curr_node_ptr = rb_ptr - curr_rb_node_offset - offsetof(Arena, node_pool[0]) - offsetof(Arena_List_Node, arena);
    Arena_List_Node *curr_node = (Arena_List_Node *)curr_node_ptr;

    // add curr_node back in to linked list
    RB_Node *head = (RB_Node *)curr_node->arena.arena_header.free_tree.root;
    RB_Node *itr = head;

    size_t counter = 0;

    // if size larger than 2k
    if (itr->size > MAX_SIZE)
    {
        unmap_arena_list_node(curr_node);
        return;
    }

    // if front has equal size
    if (itr->size == freed_node_size)
    {
        curr_rb_node->right = itr;
        curr_rb_node->parent = NULL;
        itr->parent = curr_rb_node;
        head = curr_rb_node;
        curr_node->arena.arena_header.free_tree.root = head;
        update_table(curr_node, freed_node_size, DELETE_FROM_TABLE);

        // then check table 0, arena node list stuff !!! should I do this here or below
        if (true == check_zero_used(curr_node))
        {
            unmap_arena_list_node(curr_node);
            return;
        }

        return;
    }

    while (itr->size != freed_node_size && counter < rb_node_table[NODE_TABLE_SIZE - 1] - 1)
    {
        counter++;
        char *temp = (char *)itr + sizeof(RB_Node);
        itr = (RB_Node *)temp;

        if (itr->parent == NULL && itr->right == NULL && counter < rb_node_table[NODE_TABLE_SIZE - 1] - 1)
        {
            counter++;
            temp = (char *)itr + sizeof(RB_Node);
            itr = (RB_Node *)temp;
        }
    }
    // move forward one more node if not the end of the list and parent + right are NULL
    // if(counter < rb_node_table[NODE_TABLE_SIZE - 1] - 1)
    if (itr->parent == NULL && itr->right == NULL && counter < rb_node_table[NODE_TABLE_SIZE - 1] - 1)
    {
        char *temp = (char *)itr + sizeof(RB_Node);
        itr = (RB_Node *)temp;
    }

    // if at end
    if (itr->right == NULL && itr->size < freed_node_size)
    {
        itr->right = curr_rb_node;
        curr_rb_node->parent = itr;
        curr_rb_node->right = NULL;
        update_table(curr_node, freed_node_size, DELETE_FROM_TABLE);

        if (true == check_zero_used(curr_node))
        {
            unmap_arena_list_node(curr_node);
        }

        return;
    }

    // else if(itr->right == NULL && itr->size >= freed_node_size){
    //             //SAME AS MIDDLE
    // }

    // if in middle

    else
    {
        curr_rb_node->right = itr;
        curr_rb_node->parent = itr->parent;
        RB_Node *c_parent = curr_rb_node->parent;
        // itr->parent.right = curr_rb_node;
        c_parent->right = curr_rb_node;
        itr->parent = curr_rb_node;
        update_table(curr_node, freed_node_size, DELETE_FROM_TABLE);

        if (true == check_zero_used(curr_node))
        {
            unmap_arena_list_node(curr_node);
        }

        return;
    }
    perror("Unable to free memory\n");

    return;
}

void *my_calloc(size_t nmemb, size_t size)
 {
    if(nmemb == 0 || size == 0){
        return NULL;
    }
    size_t m_size = nmemb * size;
    char *ptr = (char *)my_malloc(m_size);
    memset(ptr, 0, m_size);
    return (void *)ptr;
}

void *my_realloc(void *ptr, size_t size){
    Chunk_Header *ptr_chunk = (Chunk_Header *)(ptr - sizeof(Chunk_Header));

    if(ptr == NULL && size == 0){
        return NULL;
    }
    else if (ptr == NULL && size != 0){
        ptr = my_malloc(size);
        return ptr;
    }
    else if(ptr && size == 0){
        my_free(ptr);
        return NULL;
    }
    else if(ptr && size <= ptr_chunk->size){
        //keep same chunk
        return ptr;
    }
    else if(ptr && size > ptr_chunk->size){
        void *new_ptr = my_malloc(size);
        memcpy(new_ptr, ptr, ptr_chunk->size);
        my_free(ptr);
        return new_ptr;
    }
    else{
        perror("Unable to reallocate\n");
    }
    return NULL;
}

int main(void)
{
    char *ptr1 = my_malloc(32 * sizeof(char));
    print_hex(ptr1, 32 * sizeof(char));
    printf("ptr1 string pointer address: %p\n", ptr1);

    strcpy(ptr1, "Hello world!\n");

    printf("ptr1 string b4 realloc: %s\n", ptr1);
    char *orig = ptr1;
    ptr1 = my_realloc(ptr1, 32);
    printf("ptr1 string pointer address: %p\n", ptr1);
    printf("ptr1 string after realloc(32): %s\n", ptr1);

    ptr1 = my_realloc(ptr1, 64);
    printf("ptr1 string pointer address: %p\n", ptr1);
    printf("ptr1 string after realloc(64): %s\n", ptr1);
    printf("orig pointer address after realloc(64): %p\n", orig);
    printf("orig string: %s\n", orig);

    my_free(ptr1);

    return 0;
}