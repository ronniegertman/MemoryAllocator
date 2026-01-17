#define _GNU_SOURCE
#include "customAllocator.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h> //for memset
#include <unistd.h> //for sbrk


void test_malloc_free_1() {
    void* heapStart = sbrk(0);
    void* ptr = customMalloc(4);
    if(ptr == NULL){
        printf("malloc failed\n");
        return;
    }
    void* newHeapStart = sbrk(0);
    memset(ptr, 0, 4);
    customFree(ptr);
    printf("heapStart: %p, newHeapStart: %p\n", heapStart, newHeapStart);
    printf("ptr: %p\n", ptr);
}

void test_malloc_free_2() {
    void* heapStart = sbrk(0);
    void* ptr1 = customMalloc(4);
    void* heapAfter1 = sbrk(0);
    void* ptr2 = customMalloc(4);
    void* heapAfter2 = sbrk(0);
    if(ptr1 == NULL || ptr2 == NULL){
        printf("malloc failed\n");
        return;
    }
    memset(ptr1, 0, 4);
    memset(ptr2, 0, 4);
    customFree(ptr1);
    void* heapAfterRemove1 = sbrk(0);
    void* ptr3 = customMalloc(4);
    if(ptr3 == NULL){
        printf("malloc failed\n");
        return;
    }
    void* heapAfter3 = sbrk(0);
    memset(ptr3, 0, 4);
    customFree(ptr2);
    customFree(ptr3);
    void* heapAtEnd = sbrk(0);
    printf("heapStart: %p, heapAfter1: %p, heapAfter2: %p, heapAfterRemove1: %p, heapAfter3: %p, heapAtEnd: %p\n", heapStart, heapAfter1, heapAfter2, heapAfterRemove1, heapAfter3, heapAtEnd);
    printf("ptr1: %p, ptr2: %p, ptr3: %p\n", ptr1, ptr2, ptr3);
}

void test_malloc_free_3() {
    void* heapStart = sbrk(0);
    void* ptr1 = customMalloc(4);
    void* heapAfter1 = sbrk(0);
    void* ptr2 = customMalloc(8);
    void* heapAfter2 = sbrk(0);
    void* ptr3 = customMalloc(4);
    void* heapAfter3 = sbrk(0);
    if(ptr1 == NULL || ptr2 == NULL || ptr3 == NULL){
        printf("malloc failed\n");
        return;
    }
    memset(ptr1, 0, 4);
    memset(ptr2, 0, 8);
    memset(ptr3, 0, 4);
    customFree(ptr1);
    void* heapAfterRemove1 = sbrk(0);
    customFree(ptr2);
    void* heapAfterRemove2 = sbrk(0);
    void* ptr4 = customMalloc(12);
    customFree(ptr3);
    customFree(ptr4);
    void* heapAtEnd = sbrk(0);
    printf("heapStart: %p, heapAfter1: %p, heapAfter2: %p, heapAfter3: %p, heapAfterRemove1: %p, heapAfterRemove2: %p, heapAtEnd: %p\n", heapStart, heapAfter1, heapAfter2, heapAfter3, heapAfterRemove1, heapAfterRemove2, heapAtEnd);
    printf("ptr1: %p, ptr2: %p, ptr3: %p, ptr4: %p\n", ptr1, ptr2, ptr3, ptr4);
}

void test_malloc_free_4() {
    void* heapStart = sbrk(0);
    void* ptr1 = customMalloc(7);    // block 1
    void* ptr2 = customMalloc(16);   // block 2
    void* ptr3 = customMalloc(12);   // block 3
    void* ptr4 = customMalloc(20);   // block 4
    void* heapAfter4 = sbrk(0);

    if(ptr1 == NULL || ptr2 == NULL || ptr3 == NULL || ptr4 == NULL){
        printf("malloc failed\n");
        return;
    }

    memset(ptr1, 0xA1, 7);
    memset(ptr2, 0xB2, 16);
    memset(ptr3, 0xC3, 12);
    memset(ptr4, 0xD4, 20);

    customFree(ptr1); // free block 1
    customFree(ptr3); // free block 3

    void* heapAfterFree1And3 = sbrk(0);

    // Now allocate a block of size that fits in the smaller free space (block 1, 8 bytes), e.g., 6 bytes
    void* ptr5 = customMalloc(6);

    void* heapAfter5 = sbrk(0);

    // Check which block ptr5 was mapped to
    printf("ptr1: %p, ptr2: %p, ptr3: %p, ptr4: %p, ptr5: %p\n", ptr1, ptr2, ptr3, ptr4, ptr5);
    printf("heapStart: %p, heapAfter4: %p, heapAfterFree1And3: %p, heapAfter5: %p\n", heapStart, heapAfter4, heapAfterFree1And3, heapAfter5);

    if (ptr5 == ptr1) {
        printf("ptr5 was mapped to freed block1's memory.\n");
    } else if (ptr5 == ptr3) {
        printf("ptr5 was mapped to freed block3's memory.\n");
    } else {
        printf("ptr5 was mapped to a different memory location.\n");
    }

    // Cleanup: free all
    customFree(ptr2);
    customFree(ptr4);
    customFree(ptr5);
}
void test_calloc(){
    void* heapStart = sbrk(0);
    void* ptr1 = customCalloc(4, 4);
    void* heapAfter1 = sbrk(0);
    if(ptr1 == NULL){
        printf("calloc failed\n");
        return;
    }
    memset(ptr1, 0, 4 * 4);
    customFree(ptr1);
    void* heapAtEnd = sbrk(0);
    printf("heapStart: %p, heapAfter1: %p, heapAtEnd: %p\n", heapStart, heapAfter1, heapAtEnd);
    printf("ptr1: %p\n", ptr1);
}

// genreate  function tests for realloc:
// 1. samity
// 2. create a few blocks and shrink the last block
// 3. shrink a block from the middle
// 4. extend block from the middle 
// 5. extend last block

// 1. Sanity test for realloc: realloc on NULL ptr acts as malloc, realloc with size 0 acts as free
void test_realloc_sanity() {
    printf("==== test_realloc_sanity ====\n");
    void* heapStart = sbrk(0);

    void* ptr = customRealloc(NULL, 32); // should allocate 32 bytes
    if (!ptr) {
        printf("realloc(NULL, 32) failed\n");
        return;
    }
    memset(ptr, 0, 32);

    void* heapAfterAlloc = sbrk(0);
    ptr = customRealloc(ptr, 34);
    void* heapAfterRealloc = sbrk(0);

    // realloc with size 0 should free
    void* ret = customRealloc(ptr, 0);
    void* heapAfterFree = sbrk(0);

    printf("heapStart: %p, heapAfterAlloc: %p, heapAfterRealloc: %p, heapAfterFree: %p\n", heapStart, heapAfterAlloc, heapAfterRealloc, heapAfterFree);
}

// 2. Create a few blocks and shrink the last block
void test_realloc_shrink_last_block() {
    printf("==== test_realloc_shrink_last_block ====\n");
    void* heapStart = sbrk(0);

    void* p1 = customMalloc(32);
    void* p2 = customMalloc(40);
    void* p3 = customMalloc(48);

    void* heapAfter = sbrk(0);

    // Shrink last block
    void* p3_re = customRealloc(p3, 16);

    void* heapAfterShrink = sbrk(0);

    printf("p1: %p, p2: %p, p3(original): %p, p3(shrunk): %p\n", p1, p2, p3, p3_re);
    printf("heapStart: %p, heapAfterAlloc: %p, heapAfterShrink: %p\n", heapStart, heapAfter, heapAfterShrink);

    customFree(p1);
    customFree(p2);
    customFree(p3_re);
}

// 3. Shrink a block from the middle
void test_realloc_shrink_middle_block() {
    printf("==== test_realloc_shrink_middle_block ====\n");
    void* heapStart = sbrk(0);

    void* p1 = customMalloc(32);
    void* p2 = customMalloc(40);
    void* p3 = customMalloc(48);

    void* heapAfter = sbrk(0);

    // Shrink p2 (not last block)
    void* p2_shrunk = customRealloc(p2, 16);

    void* heapAfterShrink = sbrk(0);

    printf("p1: %p, p2(original): %p, p2(shrunk): %p, p3: %p\n", p1, p2, p2_shrunk, p3);
    printf("heapStart: %p, heapAfterAlloc: %p, heapAfterShrink: %p\n", heapStart, heapAfter, heapAfterShrink);

    customFree(p1);
    customFree(p2_shrunk);
    customFree(p3);
}

// 4. Extend block from the middle
void test_realloc_extend_middle_block() {
    printf("==== test_realloc_extend_middle_block ====\n");
    void* heapStart = sbrk(0);

    void* p1 = customMalloc(24);
    void* p2 = customMalloc(28);
    void* p3 = customMalloc(32);

    void* heapAfter = sbrk(0);

    // Extend p2 (not last block)
    void* p2_extended = customRealloc(p2, 64);

    void* heapAfterExtend = sbrk(0);

    printf("p1: %p, p2(original): %p, p2(extended): %p, p3: %p\n", p1, p2, p2_extended, p3);
    printf("heapStart: %p, heapAfterAlloc: %p, heapAfterExtend: %p\n", heapStart, heapAfter, heapAfterExtend);

    customFree(p1);
    customFree(p2_extended);
    customFree(p3);
}

// 5. Extend last block
void test_realloc_extend_last_block() {
    printf("==== test_realloc_extend_last_block ====\n");
    void* heapStart = sbrk(0);

    void* p1 = customMalloc(16);
    void* p2 = customMalloc(20);
    void* p3 = customMalloc(24);

    void* heapAfter = sbrk(0);

    // Grow p3 (last block)
    void* p3_large = customRealloc(p3, 100);

    void* heapAfterExtend = sbrk(0);

    printf("p1: %p, p2: %p, p3(original): %p, p3(extended): %p\n", p1, p2, p3, p3_large);
    printf("heapStart: %p, heapAfterAlloc: %p, heapAfterExtend: %p\n", heapStart, heapAfter, heapAfterExtend);

    customFree(p1);
    customFree(p2);
    customFree(p3_large);
}





int main() {
    test_realloc_extend_last_block();
    return 0;
}