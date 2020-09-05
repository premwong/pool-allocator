#include <check.h>
#include "pool_alloc.h"

/**
 * @brief Test suite for block pool allocator. Tests functions pool_init(), pool_malloc(), pool_free()
 * Assumptions are stated in pool_alloc.c
 * 
 */

/***************begin pool_init() test section*********************************************************/
START_TEST(pool_init_unsorted)
{
    size_t sizes[5] = {34, 12, 23, 4, 105};
    bool status = pool_init(sizes, 5);
    ck_assert(status);
}
END_TEST

START_TEST(pool_init_one)
{
    size_t sizes[1] = {8};
    bool status = pool_init(sizes, 1);
    ck_assert(status);
}
END_TEST

START_TEST(pool_init_duplicates)
{
    size_t sizes[3] = {12, 12, 12};
    bool status = pool_init(sizes, 3);
    ck_assert(status);
}
END_TEST

START_TEST(pool_init_max_length)
{
    size_t sizes[255];
    for (int i = 0; i < 255; ++i)
    {
        sizes[i] = i + 1;
    }
    bool status = pool_init(sizes, 255);
    ck_assert(status);
}
END_TEST

START_TEST(pool_init_equal_division)
{
    //Heap size: 65536 
    //65536 / 4 = 16384 (subtract 3 for header length)
    size_t sizes[1] = {16381};
    bool status = pool_init(sizes, 1);
    ck_assert(status);
}
END_TEST

START_TEST(pool_init_max_blocksize)
{
    size_t sizes[1] = {65533};
    bool status = pool_init(sizes, 1);
    ck_assert(status);
}
END_TEST

START_TEST(pool_init_null)
{
    bool status = pool_init(NULL, 0);
    ck_assert(!status);
}
END_TEST

START_TEST(pool_init_length_too_large)
{
    size_t sizes[256];
    for (int i = 0; i < 256; ++i)
    {
        sizes[i] = i + 1;
    }
    bool status = pool_init(sizes, 256);
    ck_assert(!status);
}
END_TEST

START_TEST(pool_init_blocksize_0)
{
    size_t sizes[5] = {1, 4, 3, 0 ,2};
    bool status = pool_init(sizes, 5);
    ck_assert(!status);
}
END_TEST

START_TEST(pool_init_blocksize_too_large)
{
    size_t sizes[1] = {65534};
    bool status = pool_init(sizes, 1);
    ck_assert(!status);
}
END_TEST

START_TEST(pool_init_total_blocksize_too_large)
{
    size_t sizes[5] = {1, 5000, 35300, 29500 ,2};
    bool status = pool_init(sizes, 5);
    ck_assert(!status);
}
END_TEST

Suite* pool_init_suite(void) 
{
    Suite* suite = suite_create("pool_init");
    TCase* init_success = tcase_create("Success on init: return true");
    TCase* init_fail = tcase_create("Fail on init: return false");

    tcase_add_test(init_success, pool_init_unsorted);
    tcase_add_test(init_success, pool_init_one);
    tcase_add_test(init_success, pool_init_max_blocksize);
    tcase_add_test(init_success, pool_init_duplicates);
    tcase_add_test(init_success, pool_init_max_length);
    tcase_add_test(init_success, pool_init_equal_division);

    tcase_add_test(init_fail, pool_init_null);
    tcase_add_test(init_fail, pool_init_length_too_large);
    tcase_add_test(init_fail, pool_init_blocksize_0);
    tcase_add_test(init_fail, pool_init_blocksize_too_large);
    tcase_add_test(init_fail, pool_init_total_blocksize_too_large);
    
    suite_add_tcase(suite, init_success);
    suite_add_tcase(suite, init_fail);
    return suite;
}
/***************end pool_init() test section*************************************************************/

/***************begin pool_malloc() test section*********************************************************/
START_TEST(malloc_different_sizes)
{
	size_t sizes[255];
	for (int i = 0; i < 255; ++i)
    {
		sizes[i] = i + 1;
	}
	ck_assert(pool_init(sizes, 255));

	uint32_t* m1 = pool_malloc(4);
	*m1 = 0xABCDEFF;
    uint32_t m1_int = 0xABCDEFF;

	uint64_t* m2 = pool_malloc(8);
	*m2 = 0x1234567890ABC;
    uint64_t m2_int = 0x1234567890ABC;

    ck_assert_uint_eq(*m1, m1_int);
    ck_assert_uint_eq(*m2, m2_int);   
}
END_TEST

START_TEST(malloc_0)
{
    size_t sizes[2] = {8, 4};
    ck_assert(pool_init(sizes, 2));
    uint8_t* m1 = pool_malloc(0);
    ck_assert_ptr_eq(m1, NULL);
}
END_TEST

START_TEST(malloc_check_heap_addr)
{
    size_t sizes[2] = {8, 4};
    ck_assert(pool_init(sizes, 2));
    //internal testing: testing heap. gets start of heap addr
    uint8_t* heap_start = pool_malloc(8) - 3;
    pool_malloc(8);
    pool_malloc(8);
    uint64_t* m1 = pool_malloc(8);

    //we malloc 3 times before m1. So heap start + (11 * 3) + 3 should be location of m1
    ck_assert_ptr_eq(heap_start + 36, m1);
}
END_TEST

START_TEST(malloc_test_uneven)
{
    size_t sizes[2] = {53360, 1};
    ck_assert(pool_init(sizes, 2));
    //internal testing: testing heap. gets start of heap addr
    uint8_t* heap_start = pool_malloc(2) - 3;
    
    //m1 should be at heap start + 3 + 53360 + 3 
    uint8_t* m1 = pool_malloc(1);
    ck_assert_ptr_eq(heap_start + 53366, m1);
    
    uint8_t* m2 = pool_malloc(1);
    ck_assert_ptr_eq(heap_start + 53370, m2);
    
    uint8_t* m3 = pool_malloc(1);
    ck_assert_ptr_eq(heap_start + 53374, m3);
}
END_TEST

START_TEST(malloc_not_in_blocksizes)
{
    size_t sizes[3] = {1,2,6};
    ck_assert(pool_init(sizes, 3));
    uint32_t* m1 = pool_malloc(4);
    *m1 = 0xABCDEFF;
    uint32_t m1_int = 0xABCDEFF;
    ck_assert_uint_eq(*m1, m1_int);
}
END_TEST

START_TEST(malloc_too_large)
{
    size_t sizes[3] = {1,2,5};
    ck_assert(pool_init(sizes, 3));
    uint64_t* m1 = pool_malloc(8);
    ck_assert_ptr_eq(m1, NULL);
}
END_TEST

START_TEST(malloc_all_available)
{
    //Heap size: 65536
    //block size 1 + header = 4
    //Should be 65536 / 4 = 16384 blocks allocated
    size_t sizes[1] = {1};
    ck_assert(pool_init(sizes, 1));
    int count = 0;
	while(1)
    {
		uint8_t* ptr = pool_malloc(1);
		if (ptr != NULL)
        {
		count++;
		*ptr = 0xFF;
		}
        else
        {
			break;
		}
	}
    ck_assert_int_eq(count, 16384);
    uint8_t* ptr2 = pool_malloc(1);
    ck_assert_ptr_eq(ptr2, NULL);
}
END_TEST

START_TEST(malloc_with_duplicate_sizes)
{
    //Heap size: 65536
    //block size: 65536 / 128 = 512. set size to 509 for 3 byte header
    //Should be 65536 / 4 = 16384 blocks allocated
    size_t sizes[128];
    for (int i = 0; i < 128; ++i)
    {
        sizes[i] = 509;
    }
    ck_assert(pool_init(sizes, 1));
    int count = 0;
	while(1)
    {
		uint8_t* ptr = pool_malloc(1);
		if (ptr != NULL)
        {
		count++;
		*ptr = 0x32;
		}
        else
        {
			break;
		}
	}
    ck_assert_int_eq(count, 128);
    uint8_t* ptr2 = pool_malloc(1);
    ck_assert_ptr_eq(ptr2, NULL);
}
END_TEST

START_TEST(malloc_max_size)
{
    size_t sizes[1] = {65533};
    ck_assert(pool_init(sizes, 1));
    uint8_t* m1 = pool_malloc(65533);
    ck_assert_ptr_ne(m1, NULL);
}
END_TEST

START_TEST(malloc_start_and_end_of_heap)
{
	size_t sizes[2] = {65529, 1};
	ck_assert(pool_init(sizes, 2));
    //start of heap
    uint8_t* m1 = pool_malloc(65529);

    //end of heap
    uint8_t* m2 = pool_malloc(1);

    //difference should be 65529 + 3 (for header)
    ck_assert_int_eq(m2 - m1, 65532);
}
END_TEST

Suite* pool_malloc_suite(void)
{
    Suite* suite = suite_create("pool_malloc");
    TCase* pool_malloc_case = tcase_create("Testing pool_malloc() usage");
    tcase_add_test(pool_malloc_case, malloc_different_sizes);
    tcase_add_test(pool_malloc_case, malloc_check_heap_addr);
    tcase_add_test(pool_malloc_case, malloc_test_uneven);
    tcase_add_test(pool_malloc_case, malloc_all_available);
    tcase_add_test(pool_malloc_case, malloc_too_large);
    tcase_add_test(pool_malloc_case, malloc_not_in_blocksizes);
    tcase_add_test(pool_malloc_case, malloc_with_duplicate_sizes);
    tcase_add_test(pool_malloc_case, malloc_0);
    tcase_add_test(pool_malloc_case, malloc_max_size);
    tcase_add_test(pool_malloc_case, malloc_start_and_end_of_heap);
    suite_add_tcase(suite, pool_malloc_case);
    return suite;
}

/***************end pool_malloc() test section********************************************************/

/***************begin pool_free() test section********************************************************/
START_TEST(free_check_ptr_equal)
{
	size_t block_sizes[255];
	for (int i = 0; i < 255; ++i)
    {
		block_sizes[i] = i + 1;
	}
	ck_assert(pool_init(block_sizes, 255));
	uint8_t* m1 = pool_malloc(3);
	uint8_t* m2 = pool_malloc(40);

	while(1)
    {
		uint8_t* ptr = pool_malloc(1);
		if (ptr != NULL)
        {
		*ptr = 0x31;
		}
        else
        {
			break;
		}
	}
	pool_free(m1);
	pool_free(m2);
	uint8_t* m3 = pool_malloc(3);
	ck_assert_ptr_eq(m1, m3);
	uint8_t* m4 = pool_malloc(40);
	ck_assert_ptr_eq(m2, m4);
}
END_TEST

START_TEST(check_value_malloced_after_free)
{
    size_t block_sizes[4] = {50, 3, 24, 8};
    ck_assert(pool_init(block_sizes, 4));
    
    uint8_t* m1 = pool_malloc(34);
    *m1 = 0x36;
    uint8_t* m2 = pool_malloc(18);
    *m2 = 0xFF;
    pool_free(m1);
    uint8_t* m3 = pool_malloc(34);
    *m3 = 0x44;
    ck_assert_uint_eq(*m3, 0x44);
    ck_assert_uint_eq(*m1, 0x44);
}
END_TEST

//malloc all, then free all, then malloc all and check if the count matches 
START_TEST(free_after_all_malloced)
{
    size_t block_sizes[1] = {1};
    int num_blocks = 16384;
    ck_assert(pool_init(block_sizes, 1));

    //heap size: 65536 / 4
    uint8_t* pointers[num_blocks];

	for (int i = 0; i < 16384; ++i)
    {
		uint8_t* ptr = pool_malloc(1);
        *ptr = 0x31;
        pointers[i] = ptr;
        
	}
    uint8_t* check_full = pool_malloc(1);
    ck_assert_ptr_eq(check_full, NULL);

    for (int i = 0; i < num_blocks; ++i)
    {
        pool_free(pointers[i]);
    }

    int count = 0;
    while(1)
    {
		uint8_t* ptr = pool_malloc(1);
		if (ptr != NULL)
        {
		    *ptr = 0x31;
            count++;
		}
        else
        {
			break;
		}
	}
    ck_assert_int_eq(count, num_blocks);
}
END_TEST

START_TEST(free_max_size_block)
{
    size_t block_sizes[1] = {65533};
    ck_assert(pool_init(block_sizes, 1));
    uint8_t* m1 = pool_malloc(30);
    pool_free(m1);
    uint8_t* m2 = pool_malloc(644);
    ck_assert_ptr_eq(m1, m2);
}
END_TEST

START_TEST(malloc_multiple_then_free)
{
    size_t block_sizes[4] = {50, 3, 24, 8};
    ck_assert(pool_init(block_sizes, 4));
    uint8_t* m1 = pool_malloc(24);
    pool_malloc(6);
    uint8_t* m3 = pool_malloc(2);
    pool_malloc(20);
    uint8_t* m5 = pool_malloc(25);
    pool_free(m1);
    pool_free(m3);
    pool_free(m5);
    uint8_t* m7 = pool_malloc(2);
    uint8_t* m6 = pool_malloc(24);
    uint8_t* m8 = pool_malloc(25);
    ck_assert_ptr_eq(m1, m6);
    ck_assert_ptr_eq(m3, m7);
    ck_assert_ptr_eq(m5, m8);
}
END_TEST

Suite* pool_free_suite(void)
{
    Suite* suite = suite_create("pool_free");
    TCase* pool_free_case = tcase_create("Testing pool_free() usage");
    tcase_add_test(pool_free_case, free_check_ptr_equal); 
    tcase_add_test(pool_free_case, check_value_malloced_after_free);
    tcase_add_test(pool_free_case, free_after_all_malloced);
    tcase_add_test(pool_free_case, free_max_size_block);
    tcase_add_test(pool_free_case, malloc_multiple_then_free);
    suite_add_tcase(suite, pool_free_case);
    return suite;
}
/***************end pool_free() test section*******************************************************/

int main(void)
{
    Suite* init_suite = pool_init_suite();
    SRunner* runner = srunner_create(init_suite);
    srunner_add_suite(runner, pool_malloc_suite());
    srunner_add_suite(runner, pool_free_suite());
    srunner_run_all(runner, CK_NORMAL);

    int failed = srunner_ntests_failed(runner);
    int ran = srunner_ntests_run(runner);
    srunner_free(runner);
    printf("Tests failed: %d, Tests ran: %d\n", failed, ran);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
