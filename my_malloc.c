#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "arena.h"
#include "bitwise_helpers.h"

// REMEMBER TO ADD 16-BIT ALIGNMENT!!!

/*
       The malloc() function allocates size bytes and returns a pointer
       to the allocated memory.  The memory is not initialized.  If size
       is 0, then malloc() returns a unique pointer value that can later
       be successfully passed to free().

    !!! On error, these functions return NULL and set errno. (malloc, realloc, calloc)
*/

static void *arena_list_start = NULL;

uint32_t rb_idx_table[NODE_TABLE_SIZE] = {0};

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


//!!!!!!!!!!!!!LEFT OFF HERE GET_RB_NODE_SIZE SHOULD BE FIXED NOW!!!!!!!!!
void build_default_rb_node_pool(Arena_List_Node *node)
{
    RB_Node *pool = node->arena.node_pool;

    for (uint32_t i = 0; i < rb_node_table[NODE_TABLE_SIZE - 1]; i++)
    {
        //pool[i].addr >>> set in build_default_chunks_area() function
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

    for(; chunk_count < rb_node_table[NODE_TABLE_SIZE - 1]; chunk_count++)
    {
        Chunk_Header *chunk_header = (Chunk_Header *)current_header_addr;
        node->arena.node_pool[chunk_count].assoc_c_h_addr = chunk_header; //sets pool[i].addr that points to chunk header
        chunk_header->assoc_rb_node = &node->arena.node_pool[chunk_count];
        chunk_header->size = get_rb_node_size(chunk_count);
        chunk_header->flags = FREE;
        chunk_header->prev_size = chunk_header->size; // future use in coalescing

        current_header_addr += sizeof(Chunk_Header) + chunk_header->size;
    }

    return;
}


//==========================START OF RB TREE /LINKED LIST OPERATIONS=====================//


//LINKED LIST VERSION!!!// ...Need traversal, add, delete functionality
//In this version, just need to have the right pointers of each RB_Node
//connect to the next until the end
RB_Node *build_free_tree(Arena_List_Node *al_node){
    RB_Node *head = &al_node->arena.node_pool[0];
    RB_Node *itr = head;
    for(int i = 0; i < NODE_POOL_SIZE - 1; i++){
        //set right
        itr[i].right = &itr[i + 1];
        //set parent of child
        itr[i+1].parent = &itr[i];
    }

    return head;
}

void update_table(Arena_List_Node *node, size_t chunk_size, Chunk_Op op){
    size_t cs_idx = chunk_size_index(chunk_size);
    
    if(op == DELETE_FROM_TABLE){
        node->arena.arena_header.node_table.rb_node_used[cs_idx]--;
    }
    else if(op == ADD_TO_TABLE){
        node->arena.arena_header.node_table.rb_node_used[cs_idx]++;
    }

    return;
}

void update_table_with_idx(Arena_List_Node *node, size_t chunk_size_idx, Chunk_Op op){
    if(op == DELETE_FROM_TABLE){
        node->arena.arena_header.node_table.rb_node_used[chunk_size_idx]--;
    }
    else if(op == ADD_TO_TABLE){
        node->arena.arena_header.node_table.rb_node_used[chunk_size_idx]++;
    }

    return;
}

//remove node in free_tree + update table + update chunk header(LINKED LIST VERSION)

//add node in free_tree + update table + update chunk header(LINKED LIST VERSION)

//find node in free_tree + update table + update chunk header(LINKED LIST VERSION)

// TODO: need to build out free_tree functionality (ll to start then rb tree)


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ISSUES on LINE 188!!!!!!!

char *alloc_chunk_size(Arena_List_Node *node, size_t chunk_size_idx){
    //traverse linked list... may not be the most efficient for ll, but ok
    char *my_malloc_ptr = NULL; 
    
    RB_Node *root = node->arena.arena_header.free_tree.root;
    RB_Node *itr = root;
    size_t chunk_size = rb_node_size[chunk_size_idx];
    size_t num_possible_chunks = 0;
    
    for(size_t i = chunk_size_idx; i < NODE_TABLE_SIZE-1; i++){
        num_possible_chunks += rb_node_table[i];
    }

    size_t counter = 0;

    while(itr != NULL && counter < num_possible_chunks){
       if(itr->size >= chunk_size){
            Chunk_Header *ch = itr->assoc_c_h_addr;
           
            //if chunk found... set RB_Node addr, update table
            if(ch->flags == FREE){
                my_malloc_ptr = (char *)ch + sizeof(Chunk_Header);

                //Chunk_Header
                ch->flags = IN_USE;

                //remove node from ll
                RB_Node *prev = itr->parent;

                //RB_TREE >>> allocating first node... need to reset head
                //if first, reset free_tree.root
                if(prev == NULL){
                    node->arena.arena_header.free_tree.root = itr->right;
                }

                //if in list
                prev->right = itr->right;

                //RB_NODE null out right (and left) to more easily track
                itr->left = NULL;
                itr->right = NULL;
                itr->parent = NULL;
                //change color in RB Version
                
                //check chunk size index for table update
                int32_t used_chunk_size_idx = chunk_size_index(ch->size);

                //TABLE UPDATE
                update_table_with_idx(node, used_chunk_size_idx, ADD_TO_TABLE);

                break;
            }
        } 
        itr = itr->right;
        counter++;
   }
   
   return my_malloc_ptr;
}

char *alloc_arena_chunk(size_t m_size, Arena_List_Node *node)
{
    char *my_malloc_ptr = NULL;
    
    //find chunk size and starting_size_index
    int32_t cs_idx = chunk_size_index(m_size); //rb_node_table[cs_idx] = chunk size
    uint32_t *used_table = &node->arena.arena_header.node_table.rb_node_used[0];
   
    //find open chunk size
    for(size_t i = cs_idx; i < NODE_POOL_SIZE - 1; i++){
        //chunk_size not full 
        if(used_table[i] != rb_node_table[i])
        {
            //for RB Tree... find free node of that size
            my_malloc_ptr = alloc_chunk_size(node, i);
            return my_malloc_ptr;
        }
        //if chunk_size full check next node size group
    }

    return my_malloc_ptr;
}
    
char *large_allocation(size_t m_size, Arena_List_Node *node){
    char *my_malloc_ptr = NULL;

    Chunk_Header *chunk_header = (Chunk_Header *)node->chunks_start_addr;
    chunk_header->flags = NA;
    chunk_header->size = chunk_header->prev_size = m_size + sizeof(Chunk_Header);
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

    // build_free_tree() >>> return root// head for linked list
    node->arena.arena_header.free_tree.root = build_free_tree(node);

    // rb_tree >>> after building free tree, set node->arena.rb_tree = rb root node!!!

    return;
}

Arena_List_Node *create_default_arena_list_node()
{
    Arena_List_Node *node = NULL;

    void *mmap_region = mmap(0, BASE_ARENA_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    node = (Arena_List_Node *)mmap_region;
    // build arena
    build_arena(node);

    // linked list pointers
    node->next = NULL;
    node->prev = NULL;
    return node;
}

Arena_List_Node *create_custom_arena_list_node(size_t size)
{
    Arena_List_Node *node = NULL;

    // needs to have mmap of size + custom info? 1.15 * size or more exact???
    void *mmap_region = mmap(0, size + sizeof(Chunk_Header) + sizeof(Arena_List_Node), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    node = (Arena_List_Node *)mmap_region;
    node->arena.arena_header.base = &node->arena;
    node->arena.arena_header.large_alloc = true;
    node->arena.arena_header.size = size + sizeof(Chunk_Header);
    node->chunks_start_addr = (char *)node + sizeof(Arena_List_Node);

    return node;
}


void *my_malloc(size_t m_size)
//!!!Arena_List_Node *my_malloc(size_t m_size)
{
    void *my_malloc_ptr = NULL;
    
    if(0 != build_rb_idx_table()){
        perror("Unable to build rb_idx_table");
        return NULL;
    }

    //***ARENAS linked list will be 64kb

    //0) m_size 0 does what???
    if (m_size == 0)
    {
        // pass pointer that can be passed to free thus smallest possible allocation
        //return my_malloc_ptr ???
    }

    //1) NO ARENA, CREATE/MMAP ARENA

    //1.1) NO ARENA AND SMALL
    if (!arena_list_start && m_size <= MAX_SIZE)
    {
        Arena_List_Node *head = create_default_arena_list_node();
        arena_list_start = (void *)head;
        // get initial chunk and update rb_tree + table accordingly
        my_malloc_ptr = (void *)alloc_arena_chunk(m_size, head);
        if(my_malloc_ptr == NULL){
            perror("Unable to make small allocation with newly created arena");
            return NULL;
        }

       // TESTING:
       // printf("arena_list_start = %p\n", arena_list_start);    //!!!!!!!!!!!!!!!!
        
       return my_malloc_ptr;
        //!!!return head;
    }

    //1.2) NO ARENA AND BIG
    else if (!arena_list_start && m_size > MAX_SIZE)
    {
        Arena_List_Node *head = create_custom_arena_list_node(m_size);
        arena_list_start = (void *)head;

        my_malloc_ptr = (void *)large_allocation(m_size, head);

        if(my_malloc_ptr == NULL){
            perror("Unable to make small allocation with newly created arena");
            return NULL;
        }
        return my_malloc_ptr;
        //!!!return head;
    }
    
    //2) ARENA LIST EXISTS

    //2.1 ARENA LIST EXISTS and SMALL
    if (arena_list_start && m_size <= MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
        //bool alloc_success = false;

        //iterate through arena list
        while (itr != NULL)
        {
            // if LARGE SIZE custom mmap alloc... skip to next arena
            if (itr->arena.arena_header.large_alloc == true)
            {
                itr = itr->next;
                continue;
            }

            else if(itr->arena.arena_header.large_alloc == false)
            {
                my_malloc_ptr = (void *)alloc_arena_chunk(m_size, itr);
                if(my_malloc_ptr != NULL){
                    return my_malloc_ptr;
                }
            }
            //if chunk not found, go to next arena
            itr = itr->next;
        }

        // if not found in arena linked list
        itr->next = create_default_arena_list_node();
        my_malloc_ptr = alloc_arena_chunk(m_size, itr->next);

        return my_malloc_ptr;
        //!!!return itr->next;
    }

    //2.2 ARENA LIST EXISTS and LARGE 
    else if (arena_list_start && m_size > MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
        Arena_List_Node *node = create_custom_arena_list_node(m_size);
        my_malloc_ptr = large_allocation(m_size, node);

        for (; itr != NULL; itr = itr->next)
            ;
        itr->next = node;
        return my_malloc_ptr;
        //!!!return node;
    }

    ///!!!return my_malloc_ptr;

    return NULL; //!!!!
}


    // my_free
//    void free(void *ptr)
 //   {
        /*
        The free() function frees the memory space pointed to by ptr, which must have been returned by a
        previous call to malloc(), calloc() or realloc(). Otherwise, or if free(ptr) has already been called
        before, undefined behavior occurs. If ptr is NULL, no operation is performed

            1)  update the pointer table
                    IF number of current allocs == 0
                        UNMAP THE ARENA and DELETE from Linked List
                    ELSE IF the pointer table has additional allocs:
                        look at chunk size and add freed block to R-B Tree and update table (MUTEX'D???)
                    ELSE IF current allocs < 0
                        RETURN ERROR
        */
//    }

    // my_calloc
 //   void *my_calloc(size_t nmemb, size_t size)
  //  {
        /*
        //calloc(# of elements, sizeof(data))
        1) MY_MALLOC and return the pointer
        2) Use that pointer and write over the memory with 0
        3) return the pointer to calling function

        */
        // basically malloc and then zero out the memory... it may be worth carrying a flag
        // so that the my_malloc knows to zero out the memory
 //   }
    // The calloc() function allocates memory for an array of nmemb elements of size
    // bytes each and returns a pointer to the allocated memory. The memory is set to zero.
    // If nmemb or size is 0, then calloc() returns either NULL,
    // or a unique pointer value that can later be successfully passed to free().

    // my_realloc
//    void *my_realloc(void *ptr, size_t size)
//    {
        /*
        IF ptr == NULL:
            RETURN regular MY_MALLOC(size)
        IF size == 0:
            my_free(ptr);
        ELSE IF size > 0:
            IF size < BUCKET SIZE - Stuff:
                RETURN ptr (don't need to initialize the new stuff)
            ELSE IF size >= BUCKET SIZE - Stuff:
                MY_MALLOC of the next sized bucket
                COPY the malloc'd info contained into the new bucket
                FREE the OLD BUCKET
        ELSE IF size < 0:
            return NULL && set errno

        */
//    }
    /*
    The realloc() function changes the size of the memory block pointed to by ptr to size bytes.
    The contents will be unchanged in the range from the start of the region up to the minimum of
    the old and new sizes. If the new size is larger than the old size, the added memory will not
    be initialized. If ptr is NULL, then the call is equivalent to malloc(size), for all values of size;
    if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
    Unless ptr is NULL, it must have been returned by an earlier call to malloc(), calloc() or realloc().
    If the area pointed to was moved, a free(ptr) is done.
    */

    // int main(void)
    // {
    //     char *test = my_malloc(20 * sizeof(char));
    //     if(!test){
    //         printf("Error allocating memory.\n");
    //     }

    //     printf("test string: %s", test);
    //     printf("test string pointer address: %p", test);
    //     return 0;
    // }