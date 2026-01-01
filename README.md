# Welcome to My Malloc
***

## Task
Red + Black Tree and Linked List Implementations of custom C memory allocator that uses memory mapping to obtain heap memory for the allocations. 

## Description
This memory allocator has been implemented using a linked list of 64kb arenas and custom-sized arenas for allocations of greater than 2048 bytes dependent on the program allocation requirements. Within the default arenas, a pre-set pool of 32 byte, 64 byte, 128 byte, 256 byte, 512 byte, 1024 byte and 2048 byte memory chunks along with necessary metadata are stored in order to manage allocations. The free and in-use nodes are tracked through the node_pool metadata using a linked list or red-black tree structure depending on which implementation that you choose. 

![alt text for screen readers](/memory_alloc.jpg)

## Installation

1) Download repository:

```bash
git clone git@github.com:tmooney84/my_malloc.git
```

2) Navigate to linked list or red-black tree version of the library
```
cd ll_version

-or-

cd rb_version
```

3) Compile Code
```
make
```

4) Include my_malloc.h in pre-processor directive
```
#include "my_malloc.h"
```

5) Run my_malloc executable file to seem demonstration:
```
./my_malloc
```


## Usage

void *my_malloc(size_t m_size) >>> can be used in place of a standard implementation of malloc()

void my_free(void *ptr) >>> can be used along side my_malloc() in place of free()

void *my_calloc(size_t nmemb, size_t size) >>> can be used alongside my_malloc() in place of calloc()

void *my_realloc(void *ptr, size_t size) >>> can be used alongside my_malloc() in place of my_realloc()


### The Core Team


<span><i>Made at <a href='https://qwasar.io'>Qwasar SV -- Software Engineering School</a></i></span>
<span><img alt='Qwasar SV -- Software Engineering School's Logo' src='https://storage.googleapis.com/qwasar-public/qwasar-logo_50x50.png' width='20px' /></span>
