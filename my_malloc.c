#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "arena.h"
#include "bitwise_helpers.c"

// REMEMBER TO ADD 16-BIT ALIGNMENT!!!

/*
       The malloc() function allocates size bytes and returns a pointer
       to the allocated memory.  The memory is not initialized.  If size
       is 0, then malloc() returns a unique pointer value that can later
       be successfully passed to free().

    !!! On error, these functions return NULL and set errno. (malloc, realloc, calloc)
*/
static void *arena_list_start = NULL;

void build_rb_idx_table()
{
    int idx = -1;
    for (int i = 0; i < NODE_TABLE_SIZE - 1; i++)
    {
        idx += rb_node_table[i];
        rb_idx_table[i] = idx;
    }

    // should give 193 as the last index
    rb_idx_table[NODE_TABLE_SIZE - 1] = rb_node_table[NODE_TABLE_SIZE - 1] - 1;

    return;
}

size_t get_rb_node_size(int idx)
{
    for (int i = 0; i < NODE_TABLE_SIZE - 2; i++)
    {
        if (idx >= rb_idx_table[i] && idx < rb_idx_table[i + 1] && i < NODE_TABLE_SIZE - 3)
        {
            return rb_node_table[i];
        }
        else if (idx >= rb_idx_table[i] && idx <= rb_idx_table[i + 1] && i == NODE_TABLE_SIZE - 2)
        {
            return rb_node_table[i];
        }
        else
            perror("Error getting rb_node_size\n");
        return -1;
    }
    perror("Error getting rb_node_size\n");
    return -1;
}

void set_default_arena_header(Arena_List_Node *node)
{
    node->arena.arena_header.base = node;
    node->arena.arena_header.large_alloc = false;
    node->arena.arena_header.size = BASE_ARENA_SIZE;
    node->arena.arena_header.free_tree.root = NULL;
    node->arena.arena_header.rb_node_pool = node->arena.node_pool;
    node->arena.arena_header.chunks_start_addr = node + sizeof(Arena_List_Node);
    memset(node->arena.arena_header.node_table.rb_node_used, 0, NODE_TABLE_SIZE * sizeof(uint32_t));
    return;
}

void build_default_rb_node_pool(Arena_List_Node *node)
{
    RB_Node *pool = node->arena.node_pool;

    for (int i = 0; i < rb_node_table[NODE_TABLE_SIZE - 1]; i++)
    {
        //pool[i].addr >>> set in build_default_chunks_area() function
        pool[i].size = get_rb_node_size(i);
        pool[i].left = NULL;
        pool[i].right = NULL;
        pool[i].parent = NULL;
        pool[i].color = RED;
    }

    return;
}

void build_default_chunks_area(Arena_List_Node *node)
{
    void *current_header_addr = node->arena.arena_header.chunks_start_addr;
    size_t chunk_count = 0;
    for (chunk_count; chunk_count < rb_node_table[NODE_TABLE_SIZE - 1]; chunk_count++)
    {
        Chunk_Header *chunk_header = (Chunk_Header *)current_header_addr;
        node->arena.node_pool[chunk_count].addr = current_header_addr; //sets pool[i].addr that points to chunk
        chunk_header->size = get_rb_node_size(chunk_count);
        chunk_header->flags = FREE;
        chunk_header->prev_size = chunk_header->size; // future use in coalescing

        current_header_addr += sizeof(Chunk_Header) + chunk_header->size;
    }
}


//==========================START OF RB TREE /LINKED LIST OPERATIONS=====================//


//LINKED LIST VERSION!!!// ...Need traversal, add, delete functionality
//In this version, just need to have the right pointers of each RB_Node
//connect to the next until the end
RB_Node *build_free_tree(Arena_List_Node *al_node){
    RB_Node *itr = &al_node->arena.node_pool[0];
    for(int i = 0; i < NODE_POOL_SIZE - 1; i++){
        //set right
        itr[i].right = &itr[i + 1];
        //set parent of child
        itr[i+1].parent = &itr[i];
    }

    return itr;
}

void update_table(Arena_List_Node *node, size_t chunk_size, Chunk_Op op){
    size_t cs_idx = chunk_size_index(chunk_size);
    
    if(op == DELETE_FROM_TABLE){
        node->arena.arena_header.node_table.rb_node_used[cs_idx--];
    }
    else if(op == ADD_TO_TABLE){
        node->arena.arena_header.node_table.rb_node_used[cs_idx++];
    }

    return;
}

//remove node in free_tree + update table + update chunk header(LINKED LIST VERSION)

//add node in free_tree + update table + update chunk header(LINKED LIST VERSION)

//find node in free_tree + update table + update chunk header(LINKED LIST VERSION)

// TODO: need to build out free_tree functionality (ll to start then rb tree)




/*  TODO: traverse tree until found first node that fits, 
          pull that RB_Node from the tree;
          use node.addr (which points to chunk header)


mark chunk_header

*/
int32_t get_tree_bucket(RB_Node *root, int32_t b_type_idx){

        //update table
}

/*TODO: find bucket with best fit...
    1)look at table starting with the first bucket over the size to allocate
    ...could use bitwise and then work way up to the table?
    2)use list/rb tree to find that size
    3)use that bucket and subtract 1 from that part of the table
*/

//!!!!!!!!!!!MAYBE BETTER TO JUST GO
//THROUGH LIST UNTIL rb_node is good enough size wise use or
// rb_node->right == NULL >>> don't optimize for linked lists
//just use tables for quick check if anything can be allocated
int32_t alloc_arena_chunk(size_t m_size, Arena_List_Node *node)
{
    bool chunk_op_success = false;
    
    //find chunk size and index
    int32_t cs_idx = chunk_size_index(m_size); //rb_node_table[cs_idx] = chunk size
    int32_t start_idx = rb_idx_table[cs_idx];

    uint32_t used_table[] = node->arena.arena_header.node_table.rb_node_used;
    //check buckets of idx size if none check all chunks until need new arena
    for(start_idx; start_idx < rb_idx_table[NODE_TABLE_SIZE - 1]; start_idx++){
        //int32_t cs_idx = //find the index in rb_idx_table; (...0-6 to look up size)
        if(used_table[start_idx] >= 0                   //vvv the first index of the next chunk_size 
            && used_table[start_idx] < rb_node_table[rb_idx_table[cs_idx]]){
                /*
                ok so need to find what index the start_idx will be in rb_idx_table
                which has non_regular jumps... could have a look up table or use
            
            uint32_t k = 0;
            uint32_t next_s_idx = rb_idx_table[cs_idx + 1];
            while(curr_rb_idx < NODE_TABLE_SIZE - 1 && start_idx < next_s_idx) (...here 7){
                //if can use chunk,
                        if so remove from tree
                        find node of size
                        set left, right, parent to null
                        update_table and chunk header
                        chunk_op_success = true;
                        return 1;
                start_idx++;
            }
                next_s_idx = 
                if(next_s_idx == start_idx){
                    curr_rb_idx++;
                    rb_idx_table[k + 1]
                
                }
                
                
                */
next_s_idx >>>

            RB_Node *rb_node = remove_from_tree(node->arena.arena_header.free_tree.root, start_idx);
            //find node of size 
            //set left, right, parent to null
            update_table(node, rb_node->size, DELETE_FROM_TABLE);
            //chunk header
        //
        chunk_op_success = true;
        return 1;                     //found in arena
    }
    }

    //if(chunk_op_success = false){
    // return 0;                        //not found in arena
    //}

    // else
    // {
    //     perror("Error adding to chunk\n");
    //     return -1;                   //error        
    // }
}

//==========================END OF RB TREE /LINKED LIST OPERATIONS=====================//
void build_arena(Arena_List_Node *node)
{
    build_default_arena_header(node);
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
    void *mmap_region = mmap(0, size + sizeof(Arena_List_Node), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    node = (Arena_List_Node *)mmap_region;
    node->arena.arena_header.base = node;
    node->arena.arena_header.large_alloc = true;
    node->arena.arena_header.size = size;
    node->arena.arena_header.chunks_start_addr = node + sizeof(Arena_List_Node);

    return node;
}



void *my_malloc(size_t m_size)
{
    // builds the 2nd array for caculaculating chunk sizes
    build_rb_idx_table();

    /*look for first fit size in a table
    //***ARENAS linked list will be 64kb

    1) NO ARENA, CREATE/MMAP ARENA
        IF no arena and less than (3k):
            then create and allocate new arena of 64kb,
            */
    if (m_size == 0)
    {
        // pass pointer that can be passed to free thus smallest possible allocation
    }

    //***if no arena, create arena place on list and
    //***then return chunk addr + update rb tree and table
    if (!arena_list_start && m_size <= MAX_SIZE)
    {
        Arena_List_Node *head = create_default_arena_list_node();
        arena_list_start = (void *)head;

        // get initial chunk and update rb_tree + table accordingly
    }

    // ELSE IF no arena and > (3kb): custom arena
    else if (!arena_list_start && m_size > MAX_SIZE)
    {
        Arena_List_Node *head = create_custom_arena_list_node(m_size);
        arena_list_start = (void *)head;

    }
    //************FOR SEARCHING ARENAS IN ARENA LIST, CHECK ARENA_HEADER
    //***************FOR THE large_alloc, if TRUE, SKIP TO THE NEXT ARENA
    //***************IF THERE IS NONE AFTER, CREATE NEW DEFAULT ARENA
    /*
        2) ARENA EXISTS
        if(arena_list_start && m_size <= MAX_SIZE){
            I) go to first arena in list check if large_alloc == false
                a) if so then check table to see for 2^n larger and work
                way up
                    i) if no appropriate chunk exists
                        create new arena
                        add the m_size to the new arena
                        put the new arena at the end of the list


                b) else if large_alloc != false && ALN->next skip to next

                c) else if large_alloc != false && ALN->next == NULL
                    create new
        }
    */
    if (arena_list_start && m_size <= MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
        bool alloc_success = false;
        while (itr != NULL)
        {
            // if LARGE SIZE custom mmap alloc... skip to next arena
            if (itr->arena.arena_header.large_alloc == true)
            {
                itr = itr->next;
                continue;
            }

            else if (1 == alloc_arena_chunk(m_size, itr))
            {
                alloc_success = true;
                break;
            }

            //if chunk not found, go to next arena
            itr = itr->next;
        }

        // if not found in arena linked list
        if (alloc_success == false)
        {
            itr->next = create_default_arena_list_node();
            add_to_chunk_in_arena(m_size, itr->next);
        }
    }

    else if (arena_list_start && m_size > MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
        Arena_List_Node *node = create_custom_arena_list_node(m_size);

        for (; itr != NULL; itr = itr->next)
            ;
        itr->next = node;
    }
}

    // my_free
    void free(void *ptr)
    {
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
    }

    // my_calloc
    void *my_calloc(size_t nmemb, size_t size)
    {
        /*
        //calloc(# of elements, sizeof(data))
        1) MY_MALLOC and return the pointer
        2) Use that pointer and write over the memory with 0
        3) return the pointer to calling function

        */
        // basically malloc and then zero out the memory... it may be worth carrying a flag
        // so that the my_malloc knows to zero out the memory
    }
    // The calloc() function allocates memory for an array of nmemb elements of size
    // bytes each and returns a pointer to the allocated memory. The memory is set to zero.
    // If nmemb or size is 0, then calloc() returns either NULL,
    // or a unique pointer value that can later be successfully passed to free().

    // my_realloc
    void *my_realloc(void *ptr, size_t size)
    {
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
    }
    /*
    The realloc() function changes the size of the memory block pointed to by ptr to size bytes.
    The contents will be unchanged in the range from the start of the region up to the minimum of
    the old and new sizes. If the new size is larger than the old size, the added memory will not
    be initialized. If ptr is NULL, then the call is equivalent to malloc(size), for all values of size;
    if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
    Unless ptr is NULL, it must have been returned by an earlier call to malloc(), calloc() or realloc().
    If the area pointed to was moved, a free(ptr) is done.
    */

    int main(void)
    {
        //??? should I build the rb_idx_table in my_malloc fn and others individually
        //or should I run them through main?
    // builds the 2nd array for caculaculating chunk sizes
    //build_rb_idx_table();



        return 0;
    }