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
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]--;
    }
    else if(op == ADD_TO_TABLE){
        node->arena.arena_header.node_table.rb_node_used[cs_idx]++;
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]++;
    }

    return;
}

void update_table_with_idx(Arena_List_Node *node, size_t chunk_size_idx, Chunk_Op op){
    if(op == DELETE_FROM_TABLE){
        node->arena.arena_header.node_table.rb_node_used[chunk_size_idx]--;
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]--;
    }
    else if(op == ADD_TO_TABLE){
        node->arena.arena_header.node_table.rb_node_used[chunk_size_idx]++;
        node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE - 1]++;
    }

    return;
}

//remove node in free_tree + update table + update chunk header(LINKED LIST VERSION)

//add node in free_tree + update table + update chunk header(LINKED LIST VERSION)

//find node in free_tree + update table + update chunk header(LINKED LIST VERSION)

// TODO: need to build out free_tree functionality (ll to start then rb tree)


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
                    itr->right->parent = NULL;
                }

                //if in list and not last entry of node_pool for current arena
                else if(itr->right != NULL){
                    prev->right = itr->right;
                    itr->right->parent = prev;
                }

                else{
                    prev->right = NULL;
                }


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
            counter++;
        } 
        itr = itr->right;
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
    chunk_header->flags = IN_USE;
    chunk_header->size = m_size + sizeof(Chunk_Header);
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

    //not sure why there is a segfault without an extra ~5000 bytes 
    size_t m_size = (size + sizeof(Chunk_Header) + 1 * sizeof(Arena_List_Node) + 5000);

    // needs to have mmap of size + custom info? 1.15 * size or more exact???
    void *mmap_region = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    node = (Arena_List_Node *)mmap_region;
    node->next = NULL;
    node->prev = NULL;
    node->arena.arena_header.base = &node->arena;
    node->arena.arena_header.large_alloc = true;
    node->arena.arena_header.size = m_size;
    node->arena.arena_header.rb_node_pool = node->arena.node_pool;
    node->chunks_start_addr = (char *)node + sizeof(Arena_List_Node);
    
    //only one node/chunk is used
    node->arena.node_pool[0].assoc_c_h_addr = (Chunk_Header *)node->chunks_start_addr;
    node->arena.node_pool[0].rb_node_num = 0;
    node->arena.node_pool[0].size = node->arena.arena_header.size;
    node->arena.node_pool[0].left = NULL;
    node->arena.node_pool[0].right = NULL;
    node->arena.node_pool[0].parent = NULL;
    node->arena.node_pool[0].color = NO_COLOR;

    Chunk_Header *header = (Chunk_Header *)node->chunks_start_addr;
    header->assoc_rb_node = (void *)&node->arena.node_pool[0];
    header->flags = NA;
    header->size = size;
    header->prev_size = size;

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
        
       //!!!return my_malloc_ptr;
        return head;
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
        //!!!return my_malloc_ptr;
        return head;
    }
    
    //2) ARENA LIST EXISTS

    //2.1 ARENA LIST EXISTS and SMALL
    if (arena_list_start && m_size <= MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
        Arena_List_Node *curr;
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
            curr = itr;
            itr = itr->next;
        }

        // if not found in arena linked list
        Arena_List_Node *new_node = create_default_arena_list_node();
        if(!new_node){
            perror("Unable to create new node.\n");
            return NULL;
        }
        curr->next = new_node;
        my_malloc_ptr = alloc_arena_chunk(m_size, curr->next);

        //!!!return my_malloc_ptr;
        return curr->next;
    }

    //2.2 ARENA LIST EXISTS and LARGE 
    else if (arena_list_start && m_size > MAX_SIZE)
    {
        Arena_List_Node *itr = (Arena_List_Node *)arena_list_start;
        Arena_List_Node *node = create_custom_arena_list_node(m_size);
        my_malloc_ptr = large_allocation(m_size, node);

        for (; itr->next != NULL; itr = itr->next)
            ;
        itr->next = node;
        //!!!return my_malloc_ptr;
        return node;
    }

    ///!!!return my_malloc_ptr;

    return NULL; //!!!!
}

// The free() function frees the memory space pointed to by ptr, which must have been returned by a
// previous call to malloc(), calloc() or realloc(). Otherwise, or if free(ptr) has already been called
// before, undefined behavior occurs. If ptr is NULL, no operation is performed

//my_free >>> READY TO DEBUG
//    void my_free(void *ptr)
 //   {
        /*

        if(ptr == NULL){
            return; 
        }
        
        1) need to go to the malloc'd ptr then - sizeof(Chunk_Header)

        //update Chunk_Header
        Chunk_Header *curr_head = (Chunk_Header *)(ptr - sizeof(Chunk_Header));
        curr_head->flags = FREE;

        //update RB_Node and add back in list + table
        RB_Node *curr_rb_node = (RB_Node *)curr_head->assoc_rb_node;
        size_t freed_node_num = curr_rb_node->rb_node_num;
        size_t freed_node_size = curr_rb_node->size;
        
        Arena_List_Node *curr_node = (Arena_List_Node *)(curr_rb_node - freed_node_num *sizeof(RB_Node) - sizeof(Arena_Header) - 2 * Arena_List_Node *);

//!!!!!!!!!!!!!!!!!!!!!!START LINKED LIST VERSION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //add curr_node back in to linked list
        RB_Node *head = (RB_Node *)curr_node->arena.arena_header.rb_node_pool;
        RB_Node *itr = head;

        size_t counter = 0;

        //if front has equal size
        if(itr->size == freed_node_size){
            curr_rb_node->right = itr;
            curr_rb_node->parent = NULL;
            itr->parent = curr_rb_node;
            head = curr_rb_node;
            update_table(curr_node, freed_node_size, ADD_TO_TABLE);

            //then check table 0, arena node list stuff !!! should I do this here or below
        }
   
        else if{
            while(itr != NULL && itr->size != freed_node_size && counter < rb_node_table[NODE_TABLE_SIZE - 1] - 1){
                itr = itr->right; 
                counter++;
            }
                
            //if at end
                if(itr->right == NULL && itr->size >= freed_node_size){
                    //SAME AS MIDDLE
                }
                else if(itr->right == NULL && itr->size < freed_node_size){
                    itr->right = curr_rb_node;
                    curr_rb_node->parent = itr;
                    curr_rb_node->right = NULL; 
                    update_table(curr_node, freed_node_size, ADD_TO_TABLE);
            //then check table 0, arena node list stuff !!! should I do this here or below
                }
        //if in middle
                else{
                    curr_rb_node->right = itr;
                    curr_rb_node->parent = itr->parent;
                    itr->parent.right = curr_rb_node;
                    itr->parent = curr_rb_node;
                    update_table(curr_node, freed_node_size, ADD_TO_TABLE);
            //then check table 0, arena node list stuff !!! should I do this here or below
                }
//!!!!!!!!!!!!end LINKED LIST VERSION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

            //check if arena needts to be removed
            if(curr_node->arena.arena_header.node_table.rb_node_used[NODE_TABLE_SIZE] == 0){
            //remove from arena_list

                Arena* head = (Arena *)arena_list_start;
                Arena* itr = (Arena *)arena_list_start;

                //first node
                if(itr == curr_node){
                    itr->next->prev = NULL;
                    arena_list_start = (void *)itr->next; 
                }
                while(itr != curr_node && itr != NULL){
                    itr = itr->next; 
                }
                
                //in middle
                if(itr == curr_node && itr->next != NULL){
                    itr->prev->next = itr->next;
                    itr->next->prev = itr->prev;
                }
                else if(itr == curr_node && itr->next == NULL){
                    itr->prev->next = NULL;
                }

                else{
                    perror("Unable to find arena for removal"); 
                    return;
                }
            
                if(-1 == munmap((void *)curr_node, curr_node->arena.arena_header.size)){
                    perror("Unable to unmap arena from memory\n");
                    return; 
                }
            }

            return;
        }

        
       
        


        //need to add back into list for right and parent



        2) then cast address to (Chunk Header)... update header
        3) jump to rb node, update that too
        4) update used_table
        5) then check the used_table[NODE_TABLE_SIZE - 1] == 0
                if used_table[NODE_TABLE_SIZE - 1] < 0 RETURN ERROR
                if so then can then can unmap memory of the current ARENA_LIST_NODE 
                and update arena_list_start LIST... so delete node in middle end or beginning
                    which sets arena_list_start == NULL;





            // 1)  update the pointer table
            //         IF number of current allocs == 0
            //             UNMAP THE ARENA and DELETE from Linked List
            //         ELSE IF the pointer table has additional allocs:
            //             look at chunk size and add freed block to R-B Tree and update table (MUTEX'D???)
            //         ELSE IF current allocs < 0
            //             RETURN ERROR
        */
//    }






    // my_calloc >>> READY TO DEBUG
 //   void *my_calloc(size_t nmemb, size_t size)
  //  {
        //if(nmemb == 0 || size == NULL){
            //return NULL;
        //}
        //size_t m_size = nmemb * size;
        //char *ptr = (char *)my_malloc(m_size);
        //memset(ptr, 0, m_size);
        //return (void *)ptr;
    //}


/*
        //calloc(# of elements, sizeof(data))
        1) MY_MALLOC and return the pointer
        2) Use that pointer and write over the memory with 0
        3) return the pointer to calling function

        // basically malloc and then zero out the memory... it may be worth carrying a flag
        // so that the my_malloc knows to zero out the memory
 //   }
    // The calloc() function allocates memory for an array of nmemb elements of size
    // bytes each and returns a pointer to the allocated memory. The memory is set to zero.
    // If nmemb or size is 0, then calloc() returns either NULL,
    // or a unique pointer value that can later be successfully passed to free().
*/







    // my_realloc >>>READY TO DEBUG
// void *my_realloc(void *ptr, size_t size){
//     Chunk_Header *ptr_chunk = (Chunk_Header *)(ptr - sizeof(Chunk_Header));

//     if(ptr == NULL && size == 0){
//         return NULL;
//     }
//     else if (ptr == NULL && size != 0){
//         ptr = my_malloc(size);
//         return ptr;
//     }
//     else if(ptr && size == 0){
//         my_free(ptr);
//         return NULL;
//     }
//     else if(ptr && size <= ptr_chunk->size){
//         //keep same chunk
//         return ptr;
//     }
//     else if(ptr && size > ptr_chunk->size){
//         void *new_ptr = my_malloc(size);
//         memcpy(new_ptr, ptr, ptr_chunk->size);
//         my_free(ptr);
//         return new_ptr;
//     }
//     else if(size < 0){
//         perror("Invalid realloc size\n");
//         return NULL;
//     }
//     else{
//         perror("Unable to reallocate\n");
//     }
//     return NULL;
// }






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