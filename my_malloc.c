#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "arena.h"


//REMEMBER TO ADD 16-BIT ALIGNMENT!!!

/*
       The malloc() function allocates size bytes and returns a pointer
       to the allocated memory.  The memory is not initialized.  If size
       is 0, then malloc() returns a unique pointer value that can later
       be successfully passed to free(). 

    !!! On error, these functions return NULL and set errno. (malloc, realloc, calloc)
*/
static void* arena_list_start = NULL;

void build_rb_idx_table(){
    int idx = -1;
   for(int i = 0; i < NODE_TABLE_SIZE - 1; i++){
        idx += rb_node_table[i];
        rb_idx_table[i] = idx;
   } 

   //should give 197 as the last index
   rb_idx_table[NODE_TABLE_SIZE - 1] = rb_node_table[NODE_TABLE_SIZE - 1] - 1;

   return;
}

size_t get_rb_node_size(int idx){
   for(int i = 0; i < NODE_TABLE_SIZE - 2; i++){
    if(idx >= rb_idx_table[i] && idx < rb_idx_table[i + 1] && i < NODE_TABLE_SIZE - 3){
        return rb_node_table[i];
    }
    else if(idx >= rb_idx_table[i] && idx <= rb_idx_table[i + 1] && i == NODE_TABLE_SIZE - 2){
        return rb_node_table[i];
    }
    else
        perror("Error getting rb_node_size\n");
        return -1;
   }
        perror("Error getting rb_node_size\n");
        return -1;
}

void build_default_rb_node_pool(Arena_List_Node *node){
    RB_Node *pool = node->arena.node_pool;

    for(int i = 0; i < rb_node_table[NODE_TABLE_SIZE -1]; i++){
       pool[i].addr = pool + (i * sizeof(RB_Node));
       pool[i].size =  get_rb_node_size(i);
       pool[i].left = NULL;
       pool[i].right = NULL;
       pool[i].parent = NULL;
       pool[i].color = NO_COLOR;
    }

    return;
}

//void set_default_arena_header(Arena_Header *header, void *mmap_region){
void set_default_arena_header(Arena_List_Node *node){
        node->arena.arena_header.base = node;
        node->arena.arena_header.large_alloc = false; 
        node->arena.arena_header.size = BASE_ARENA_SIZE;
        node->arena.arena_header.free_tree.root = NULL;
        node->arena.arena_header.rb_node_pool = node->arena.node_pool; 
        memset(node->arena.arena_header.node_table.rb_node_used, 0, NODE_TABLE_SIZE * sizeof(uint32_t));
        return;
}

void build_arena(Arena_List_Node *node){
    build_default_rb_node_pool(node);

    //build chunk and chunk headers(headers outside of chunks)
    //set default Arena_Header
    set_default_arena_header(node);
        

}

Arena_List_Node *create_arena_list_node(size_t size){
    if(size < LARGE_SIZE){
        void *mmap_region = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0); 
        Arena_List_Node *node = (Arena_List_Node *)mmap_region;
        
        //build arena
        build_arena(node);
        
        //linked list pointers
        node->next = NULL;
        node->prev = NULL;
        return;
    }
    else if(size >= LARGE_SIZE){

    }
    else{
        perror("Error... incompatible size.\n");
        return NULL;
    }
   

   return node;
}

void *my_malloc(size_t m_size){
    //builds the 2nd array for caculaculating chunk sizes     
    build_rb_idx_table();

    /*look for first fit size in a table
    //***ARENAS linked list will be 64kb

    1) NO ARENA, CREATE/MMAP ARENA 
        IF no arena and less than (3k):
            use 8kb
            then create and allocate new arena of 64kb,
            with the golden ratio to fill the remaining buckets rounding up to next 4kb... 
            so if (2.5k allocation + stuff) then 4kb so fill the remaining 60kb with the 8K golden ratio
            */
           if(m_size == 0){
            //pass pointer that can be passed to free thus smallest possible allocation
           }
            
            if(arena_list_start = NULL){
                    Arena_List_Node *head = NULL;
                    head = create_arena_list_node(m_size);
                    arena_list_start = (void *)head;
                    Arena_List_Node *current_arena = add_to_arena_list(m_size);
                    //^^^LEFT OFF HERE
            }


/*
        ELSE IF no arena and >= (3kb):
            mmap directly >>> 

        ELSE IF no arena and >= (~54kb):
            create an arena that is a multiple of 64kb and is > than space to map +
            build out remaining areas with the golden ratio with remaining areas

        ELSE 
            return NULL and set errno

    2) ARENA EXISTS
        IF < 2kb + stuff ~3kb:
            Check for best fit free buckets if under 3kb + Stuff (~4kb)
            IF THE NEXT SIZE UP EXISTS (ie: 17 bytes in 32 byte buckets is full check the 64 byte buckets
            and so on until up to 3kb)
                GO INTO R-B Tree and RETRIEVE BUCKET, USE and DECREMENT Local Pointer Map
            IF ALL FULL:
                check next arena in the list
            IF NO SPACE EXISTS: 
                CREATE/MMAP NEW ARENA and add to the end of the ARENA LIST and place the
            malloc() in it

        ELSE IF >= 2kb + stuff ~3kb && < 59 + stuff ~60kb:
            CREATE NEW ARENA, ADD TO LIST and place new malloc in it, fill the rest of it with the 
            4k Golden Ratio dependent on remaining size

        ELSE IF >= 59 + stuff ~60kb:
            CREATE NEW ARENA with the size of N * 64kb where malloc rounds up to that ratio and fills the
            rest with either the 8k Golden Ratio or 4k Golden Ratio dependent on remaining size

        ELSE 
            return NULL and set errno
*/

    }




//my_free
void free(void *ptr){
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

//my_calloc
void *my_calloc(size_t nmemb, size_t size){
    /*
    //calloc(# of elements, sizeof(data))
    1) MY_MALLOC and return the pointer
    2) Use that pointer and write over the memory with 0
    3) return the pointer to calling function 
    
    */ 
    //basically malloc and then zero out the memory... it may be worth carrying a flag
    //so that the my_malloc knows to zero out the memory

}
//The calloc() function allocates memory for an array of nmemb elements of size 
//bytes each and returns a pointer to the allocated memory. The memory is set to zero. 
//If nmemb or size is 0, then calloc() returns either NULL, 
//or a unique pointer value that can later be successfully passed to free().


//my_realloc
void *my_realloc(void *ptr, size_t size){
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

int main(void){


    return 0;
}