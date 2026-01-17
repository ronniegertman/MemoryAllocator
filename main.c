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

int main() {
    test_malloc_free_3();
    return 0;
}