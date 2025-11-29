#include <stdint.h>
#include <stdio.h>
#include "arena.h"
//SHOULD I MUTEX THE TABLE UPDATES???
// GOLDEN RATIO NEEDS TO FIT INTO N * 4kb... needs to adapt to different amounts of available memory
// 4KB:            3    2   1   512     256     128     64      32  Stuff...(in bytes)
//
//
// 8KB:            3    2   1   512     256     128     64      32  Stuff...(in bytes)
//
// 


/*
       The malloc() function allocates size bytes and returns a pointer
       to the allocated memory.  The memory is not initialized.  If size
       is 0, then malloc() returns a unique pointer value that can later
       be successfully passed to free(). 

    !!! On error, these functions return NULL and set errno. (malloc, realloc, calloc)
*/
void *my_malloc(size_t size){
    /*look for first fit size in a table
    //***ARENAS linked list will be 64kb

    1) NO ARENA, CREATE/MMAP ARENA 
        IF no arena and less than (32kb - stuff):
            then create and allocate new arena of 64kb,
            with the golden ratio to fill the remaining buckets rounding up to next 4kb... 
            so if (2.5k allocation + stuff) then 4kb so fill the remaining 60kb with the 8K golden ratio

        ELSE IF no arena and >= (32kb - stuff):
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