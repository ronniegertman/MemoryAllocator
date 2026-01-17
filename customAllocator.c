#define _GNU_SOURCE
#include "customAllocator.h"
#include <stdbool.h>
#include <stdio.h> //for printf
#include <stdint.h> //for intptr_t
#include <unistd.h> //for sbrk
#include <string.h> //for memset

//TODO: check if sbrk fails when out of memory

Block* blockList = NULL; // global
Block* lastBlock = NULL; // global 

void* bestFit(size_t size){
    Block* current = blockList;
    Block* bestBlock = NULL;
    size_t bestSize = (size_t)(-1); // highest possible size
    while(current != NULL){
        if(current->free && current->size >= size && current->size < bestSize){
            bestBlock = current;
            bestSize = current->size;
        }
        current = current->next;
    }
    return bestBlock;
}

void* customMalloc(size_t size){
    size_t blockSize = ALIGN_TO_MULT_OF_4(size); // aligning only user memory
    Block* newBlock = NULL;
    if(blockList == NULL){ // empty heap
        newBlock = (Block*)sbrk((intptr_t)(blockSize + sizeof(Block)));
        if(newBlock == SBRK_FAIL){
            return NULL;
        }
        newBlock->size = blockSize;
        newBlock->free = false;
        newBlock->next = NULL;
        newBlock->prev = NULL;
        blockList = newBlock;
        lastBlock = newBlock;
        return (void*)(lastBlock + 1);
    }

    newBlock = bestFit(size);
    if(newBlock != NULL){
        newBlock->free = false;
        return (void*)(newBlock + 1);
    }

    // need to allocate new memory in the heap
    newBlock = (Block*)sbrk((intptr_t)(blockSize + sizeof(Block)));
    if(newBlock == SBRK_FAIL){
        return NULL;
    }
    newBlock->size = blockSize;
    newBlock->free = false;
    newBlock->next = NULL;
    newBlock->prev = lastBlock;
    lastBlock->next = newBlock;
    lastBlock = newBlock;
    return (void*)(lastBlock + 1);
}


static Block* findBlock(void* ptr) {
    if (ptr == NULL) {
        return NULL;
    }
    Block* current = blockList;
    while (current != NULL) {
        void* blockUserPtr = (void*)(current + 1);
        if (blockUserPtr == ptr) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// void customFree0(void* ptr){
//     if (ptr == NULL){
//         printf("<free error>: passed null pointer\n");
//         return;
//     }
//     void* heapStart = (void*)blockList;
//     void* heapEnd = sbrk(0);
//     if(ptr < heapStart || ptr >= heapEnd){
//         printf("<free error>: passed non-heap pointer 1\n");
//         return;
//     }

//     Block* block = findBlock(ptr);
//     if(block == NULL){
//         // This is a heap pointer, but not allocated by the allocator
//         printf("<free error>: passed non-heap pointer 2\n");
//         return;
//     }

//     if (block->free) {
//         // This is a heap pointer, but already freed
//         printf("<free error>: passed non-heap pointer 3\n");
//         return;
//     }

//     block->free = true;

//     if(block->next != NULL && block->next->free){
//         if(block->next == lastBlock){
//             lastBlock = block;
//         }
//         block->size += block->next->size + sizeof(Block); // new block size includes the header of the second block
//         block->next = block->next->next;
//         block->next->prev = block;
//     }
//     if(block->prev != NULL && block->prev->free){
//         if(block == lastBlock){
//             lastBlock = block->prev;
//         }
//         block->prev->size += block->size + sizeof(Block); // new block size includes the header of the original block
//         block->prev->next = block->next;
//         block->next->prev = block->prev;
//     }

//     if (block->prev == NULL) {
//         blockList = block->next;
//     }

//     if(lastBlock->free){
//         void* newLastBlock = lastBlock->prev;
//         sbrk(-1 * (lastBlock->size + sizeof(Block))); // TODO: check if we can handle sbrk failed
//         lastBlock = newLastBlock;
//         return;
//     }
// }


void customFree(void *ptr) {
    if (ptr == NULL) {
        printf("<free error>: passed null pointer\n");
        return;
    }

    // Avoid void* comparisons
    char *heapEnd = (char *)sbrk(0);

    // If blockList is NULL, allocator has no heap blocks yet
    if (blockList == NULL) {
        printf("<free error>: passed non-heap pointer\n");
        return;
    }

    // Minimal sanity: ptr must be below current program break
    if ((char *)ptr >= heapEnd) {
        printf("<free error>: passed non-heap pointer\n");
        return;
    }

    Block *block = findBlock(ptr);
    if (block == NULL) {
        printf("<free error>: passed non-heap pointer\n");
        return;
    }

    if (block->free) {
        return;
    }

    block->free = true;

    // 1) Coalesce with NEXT if free
    if (block->next != NULL && block->next->free) {
        Block *n = block->next;
        block->size += sizeof(Block) + n->size;
        block->next = n->next;
        if (block->next != NULL) {
            block->next->prev = block;
        } else {
            // merged into the last block
            lastBlock = block;
        }
    }

    // 2) Coalesce with PREV if free
    if (block->prev != NULL && block->prev->free) {
        Block *p = block->prev;
        p->size += sizeof(Block) + block->size;
        p->next = block->next;
        if (p->next != NULL) {
            p->next->prev = p;
        } else {
            lastBlock = p;
        }
        block = p; // IMPORTANT: block is now the merged block
    }

    // 3) Return memory to OS if the (merged) block is at the end
    if (block == lastBlock && block->free) {
        // Detach from list first
        if (block->prev != NULL) {
            block->prev->next = NULL;
            lastBlock = block->prev;
        } else {
            // This was the only block
            blockList = NULL;
            lastBlock = NULL;
        }

        // Shrink by the size of the last free block + header
        intptr_t shrink = (intptr_t)(sizeof(Block) + block->size);
        if (shrink > 0) {
            sbrk(-shrink); // if you want: check (void*)-1 for failure
        }
    }
}

void* customCalloc(size_t nmemb, size_t size){
    void* ptr = customMalloc(nmemb * size);
    if(ptr == NULL){
        return NULL;
    }
    memset(ptr, 0, nmemb * size);
    return ptr;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void* customRealloc(void* ptr, size_t size){
    if (ptr == NULL) {
        return customMalloc(size);
    }

    // Avoid void* comparisons
    char *heapEnd = (char *)sbrk(0);

    // Minimal sanity: ptr must be below current program break
    if ((char *)ptr >= heapEnd || (char *)ptr < (char *)blockList) {
        printf("<realloc error>: passed non-heap pointer\n");
        return NULL;
    }

    Block *block = findBlock(ptr);
    if (block == NULL) {
        printf("<realloc error>: passed non-heap pointer\n");
        return NULL;
    }
    if(size == 0){
        customFree(ptr);
        return NULL;
    }

    size_t oldSize = block->size;
    if (size <= oldSize) {
        if(block == lastBlock){
            size_t newSize = ALIGN_TO_MULT_OF_4(size);
            if(newSize == oldSize){
                return ptr;
            }
            sbrk(newSize - oldSize);
            lastBlock->size = newSize;
            return ptr;
        }
        void* newPtr = customMalloc(size);
        if(newPtr == NULL){
            return NULL;
        }
        memcpy(newPtr, ptr, size);
        customFree(ptr);
        return newPtr;
    }
    else { // size > oldSize
        void* newPtr = customMalloc(size);
        if(newPtr == NULL){
            return NULL;
        }
        memcpy(newPtr, ptr, oldSize);
        customFree(ptr);
        return newPtr;
    }
}