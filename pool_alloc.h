#ifndef __POOL_ALLOC_H__
#define __POOL_ALLOC_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define HEAP_SIZE 65536
#define MAX_BLOCK_SIZE_COUNT 255
#define METADATA_LENGTH 3

#ifndef MAX
	#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

//struct for metadata encoding in each block
typedef struct __attribute__((__packed__)) metadata
{
	uint16_t next_freelist_offset; //offset from the start of the heap to next free block (in Bytes)
	uint8_t partition_number; //index of partion in heap based on block size (eg: 0,1,2... block_size_count -1)
} metadata;

// Initialize the pool allocator with a set of block sizes appropriate for this application.
// Returns true on success, false on failure.
bool pool_init(const size_t* block_sizes, size_t block_size_count);

// Allocate n bytes.
// Returns pointer to allocate memory on success, NULL on failure.
void* pool_malloc(size_t n);

// Release allocation pointed to by ptr.
void pool_free(void* ptr);

//Comparator for qsort
static int qsort_cmp(const void* a, const void* b); 

#endif //__POOL_ALLOC_H__