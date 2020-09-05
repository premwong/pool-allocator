/**************************************************************************************************************
Assumptions: 
 - The heap is ideally partitioned evenly among the different block sizes
 - Heap size and maximum number of block sizes are limited by the defined values

Heap is first partitioned based on block size (largest to smallest), and metadata inserted for each block as defined in pool_alloc.h

Example of heap initialized with block sizes = [2]:
----------------------------------0x00 <--heap start
next freelist offset (8)
----------------------------------0x10
partition number(0)
----------------------------------0x18
empty
----------------------------------0x20
empty
----------------------------------0x28
next freelist offset (13)
----------------------------------0x38
partition number (0)
----------------------------------0x40
empty
----------------------------------0x48
empty
----------------------------------0x50

 - Next freelist offset is the next free block in the partition, offset from the start of the heap (in bytes).
 - Example above: freelist offset is 8 at 0x00, this means the next free block is at 8 * 8 = 0x40
 - Freelist offset will be 0 if there is no remaining free block in the partition.
 - Each partition has its own freelist and the head is cached in freelist_head_arr[]
 - Calling pool_free() will free the memory for allocation but not clear data in the heap.

****************************************************************************************************************/
#include "pool_alloc.h"

static uint8_t g_pool_heap[HEAP_SIZE];

//Caching the head of the freelist in each partition allows for usual O(1) allocation at the expense of memory usage
static uint16_t freelist_head_arr[MAX_BLOCK_SIZE_COUNT];

//Sorting block sizes allows for faster allocation when a certain partition is full by going to next larger block size 
//Also reduces fragmentation by partitioning for larger blocks before smaller blocks, but at expense of memory usage
static size_t sorted_block_size_arr[MAX_BLOCK_SIZE_COUNT];

static size_t block_size_count_var;
static uint8_t* g_pool_heap_start;

/**
 * @brief initialize the pool allocator
 * pool_init() will be called before pool_malloc() or pool_free()
 * 
 * @param block_sizes block sizes 
 * @param block_size_count number of block sizes
 * @return true on init success
 * @return false on init fail
 */
bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
	if (block_size_count == 0 || block_sizes == NULL || block_size_count > MAX_BLOCK_SIZE_COUNT)
	{
		return false;
	}
	uint32_t ptr_next;
	uint32_t upper_bound_offset;
	size_t block_size;
	uint32_t partition_size = HEAP_SIZE / block_size_count;
	uint32_t remaining_heap_size = HEAP_SIZE;
	uint32_t partition_start_offset = 0;

	block_size_count_var = block_size_count;
	g_pool_heap_start = g_pool_heap;

	//sort block sizes largest to smallest
	for (int j = 0; j < block_size_count; ++j)
	{
		sorted_block_size_arr[j] = block_sizes[j];
	}
	qsort(sorted_block_size_arr, block_size_count, sizeof(size_t), qsort_cmp);

	/*******************************************************************************************************
	*	Ideally, heap is partitioned into equal shares for each block size. 
	*	Equal share: remaining heap space / remaining block sizes.
	*	Partitioning rules: Starting with largest block size:
	*	 - if block size > equal share, partition enough for one block and divide up rest evenly.
	*	 - Else, partition for max available blocks that fit within equal share and divide rest up evenly.
	*/

	for (int i = 0; i < block_size_count; ++i)
	{
		block_size = sorted_block_size_arr[i];
		if (block_size == 0 || HEAP_SIZE - METADATA_LENGTH < block_size) 
		{
			return false;
		}

		//Get max blocks that will fit into equal share or one block size if larger than equal share. 
		//Reduces fragmentation between partitions
		partition_size = MAX(block_size + METADATA_LENGTH, (remaining_heap_size / (block_size_count - i)) 
			- ((remaining_heap_size / (block_size_count - i)) % (block_size + METADATA_LENGTH)));

		//If not enough space in heap to allocate a particular block size, init fails. 
		//Since block sizes are sorted largest to smallest, it will partition for all sizes if possible
		if (partition_size > remaining_heap_size)
		{
			return false;
		}

		freelist_head_arr[i] = partition_start_offset + METADATA_LENGTH; 
		upper_bound_offset = partition_start_offset + partition_size;

		//insert metadata into heap
		for (int idx = partition_start_offset; idx < upper_bound_offset; idx += (block_size + METADATA_LENGTH)) 
		{
			ptr_next = idx + block_size + (2 * METADATA_LENGTH);
			metadata* header = (metadata*) (g_pool_heap_start + idx);
			if (ptr_next < upper_bound_offset && ptr_next <= UINT16_MAX) 
			{
				header->next_freelist_offset = ptr_next;
			} 
			else 
			{
				header->next_freelist_offset = 0;
			}
			header->partition_number = i;
		}
		remaining_heap_size = HEAP_SIZE - upper_bound_offset;
		partition_start_offset = upper_bound_offset;
	}
	return true;
}

/**
 * @brief allocate smallest block that is both free and greater than or equal to n in size
 * pointer type size from caller is not larger than n
 * 
 * @param n bytes to allocate
 * @return void* pointer to allocated memory, NULL on failure
 */
void* pool_malloc(size_t n)
{
	if (n == 0 || n > sorted_block_size_arr[0])
	{
		return NULL;
	}
	uint16_t freelist_head;
	metadata* header;
	uint8_t partition_num = 0;
	bool found_partition = false;
	
	//find which partition to use starting with smallest
	for (int i = block_size_count_var - 1; i >= 0; --i)
	{
		//check if freelist head is not 0 (has a free block) and block size  >= n
		if (freelist_head_arr[i] != 0 && sorted_block_size_arr[i] >= n)
		{
			partition_num = i;
			found_partition = true;
			break;
		}
	}
	if (!found_partition)
	{
		return NULL;
	}

	freelist_head = freelist_head_arr[partition_num]; 
	header = (metadata*) (g_pool_heap_start + freelist_head - METADATA_LENGTH);
	freelist_head_arr[partition_num] = header->next_freelist_offset;
	return (void*) (g_pool_heap_start + freelist_head);
}

/**
 * @brief frees memory in the heap buffer
 * ptr to be freed must be valid and was allocated using pool_malloc()
 * 
 * @param ptr to be freed
 */
void pool_free(void* ptr)
{
	metadata* header = ((metadata*) ptr - 1);
	uint8_t partition_num = header->partition_number;
	header->next_freelist_offset = freelist_head_arr[partition_num];
	freelist_head_arr[partition_num] = (uint8_t*) ptr - g_pool_heap_start;
}

/**
 * @brief compare function for qsort
 * 
 * @param a 
 * @param b 
 * @return int 
 */
static int qsort_cmp(const void* a, const void* b)
{
	return (*(int*) b - *(int*) a);
}
