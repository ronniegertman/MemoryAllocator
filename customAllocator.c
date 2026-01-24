#define _GNU_SOURCE
#include "customAllocator.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h> //for printf
#include <stdint.h> //for intptr_t
#include <unistd.h> //for sbrk
#include <string.h> //for memset

//TODO: check if sbrk fails when out of memory

Block* blockList = NULL; // global
Block* lastBlock = NULL; // global 
MemoryArea* memoryAreaList = NULL; // global
MemoryArea* lastMemoryArea = NULL; // global
pthread_mutex_t memoryAreaListMutex;
pthread_mutex_t heapSizeModificationMutex = PTHREAD_MUTEX_INITIALIZER;

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
        memcpy(newPtr, ptr, oldSize);
        customFree(ptr);
        return newPtr;
    }
}

void freeMemoryArea(MemoryArea* memoryArea){
    // free the block list
    BlockMT* current = memoryArea->blockList;
    while(current != NULL){
        BlockMT* next = current->next;
        customFree(current);
        current = next;
    }
    // free the memory area
    customFree(memoryArea->dataPtr);
    customFree(memoryArea);
}

void freeMemoryAreaList(){
    MemoryArea* current = memoryAreaList;
    while(current != NULL){
        MemoryArea* next = current->next;
        freeMemoryArea(current);
        current = next;
    }
    memoryAreaList = NULL;
}


MemoryArea* createMemoryArea(size_t size){
    // locking before function call
    MemoryArea* newMemoryArea = (MemoryArea*)customMalloc(sizeof(MemoryArea));
    if(newMemoryArea == NULL){
        return NULL;
    }
    // Initialize the area's data
    newMemoryArea->dataPtr = (void*)customMalloc(size);
    if(newMemoryArea->dataPtr == NULL){
        customFree(newMemoryArea);
        return NULL;
    }
    // Initialize the area's block list
    newMemoryArea->blockList = (BlockMT*)customMalloc(sizeof(BlockMT));
    if(newMemoryArea->blockList == NULL){
        customFree(newMemoryArea->dataPtr);
        customFree(newMemoryArea);
        return NULL;
    }
    newMemoryArea->blockList->size = size;
    newMemoryArea->blockList->free = true;
    newMemoryArea->blockList->next = NULL;
    newMemoryArea->blockList->prev = NULL;
    newMemoryArea->blockList->dataPtr = newMemoryArea->dataPtr;

    newMemoryArea->size = size;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&newMemoryArea->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    newMemoryArea->next = NULL;

    return newMemoryArea;
}

void heapCreate(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&memoryAreaListMutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&memoryAreaListMutex);

    for (int i = 0; i < 8; i++){
        MemoryArea* newMemoryArea = createMemoryArea(4096);
        if(newMemoryArea == NULL){
            freeMemoryAreaList();
            pthread_mutex_unlock(&memoryAreaListMutex);
            return;
        }

        if(memoryAreaList == NULL){
            memoryAreaList = newMemoryArea;
        }else{
            lastMemoryArea->next = newMemoryArea;
        }
        lastMemoryArea = newMemoryArea;
    }

    pthread_mutex_unlock(&memoryAreaListMutex);
}

void heapKill(){
    pthread_mutex_lock(&memoryAreaListMutex);
    if(memoryAreaList == NULL){
        pthread_mutex_unlock(&memoryAreaListMutex);
        return;
    }
    freeMemoryAreaList();
    pthread_mutex_unlock(&memoryAreaListMutex);

    pthread_mutex_destroy(&memoryAreaListMutex);
    pthread_mutex_destroy(&heapSizeModificationMutex);
}

BlockMT* bestFitMT(MemoryArea* memoryArea, size_t size){
    BlockMT* bestBlock = NULL;
    size_t bestSize = (size_t)(-1); // highest possible size
    BlockMT* current = memoryArea->blockList;
    while(current != NULL){
        if(current->free && current->size >= size && current->size < bestSize){
            bestBlock = current;
            bestSize = current->size;
        }
        current = current->next;
    }
    return bestBlock;
}

void* customMTMalloc(size_t size, int threadId){
    if(size > 4096){
        printf("<malloc error>: requested size is too large\n");
        return NULL;
    }
    pthread_mutex_lock(&memoryAreaListMutex);
    if(memoryAreaList == NULL){
        pthread_mutex_unlock(&memoryAreaListMutex);
        return NULL;
    }

    BlockMT* bestBlock = NULL;
    MemoryArea* chosenMemoryArea = NULL;
    bool queueHeadHasNoSpace = false;
    while(memoryAreaList != NULL){
        pthread_mutex_lock(&memoryAreaList->mutex);
        bestBlock = bestFitMT(memoryAreaList, size);
        pthread_mutex_unlock(&memoryAreaList->mutex);
        if(bestBlock == NULL && !queueHeadHasNoSpace){
            // Add a new memory area to the list
            pthread_mutex_lock(&heapSizeModificationMutex);
            lastMemoryArea->next = createMemoryArea(4096);
            pthread_mutex_unlock(&heapSizeModificationMutex);
            if(lastMemoryArea->next == NULL){
                pthread_mutex_unlock(&memoryAreaListMutex);
                return NULL;
            }
            lastMemoryArea = lastMemoryArea->next;
            queueHeadHasNoSpace = true;
        }
        chosenMemoryArea = memoryAreaList;
        
        // Rotate the memory area list (putting first area last)
        lastMemoryArea->next = memoryAreaList;
        lastMemoryArea = lastMemoryArea->next;
        memoryAreaList = memoryAreaList->next;
        lastMemoryArea->next = NULL;

        if (bestBlock != NULL){
            break;
        }
    }
    pthread_mutex_lock(&chosenMemoryArea->mutex);
    pthread_mutex_unlock(&memoryAreaListMutex);

    size_t newBlockSize = bestBlock->size - ALIGN_TO_MULT_OF_4(size);
    bestBlock->free = false;
    if(newBlockSize > 0){
        // split the block into two blocks
        pthread_mutex_lock(&heapSizeModificationMutex);
        BlockMT* newBlock = (BlockMT*)customMalloc(sizeof(BlockMT));
        pthread_mutex_unlock(&heapSizeModificationMutex);
        if(newBlock == NULL){
            pthread_mutex_unlock(&chosenMemoryArea->mutex);
            return NULL;
        }
        bestBlock->size = ALIGN_TO_MULT_OF_4(size);

        newBlock->size = newBlockSize;
        newBlock->dataPtr = (void*)((char*)bestBlock->dataPtr + bestBlock->size);
        newBlock->free = true;

        newBlock->prev = bestBlock;
        newBlock->next = bestBlock->next;
        bestBlock->next = newBlock;

        if (newBlock->next != NULL){
            newBlock->next->prev = newBlock;
        }
    }

    pthread_mutex_unlock(&chosenMemoryArea->mutex);
    return (void*)(bestBlock->dataPtr);
}

MemoryArea* findMemoryArea(void* ptr){
    MemoryArea* current = memoryAreaList;
    while(current != NULL){
        if(ptr >= current->dataPtr && ptr < (void*)((char*)current->dataPtr + current->size)){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

BlockMT* findBlockMT(MemoryArea* memoryArea, void* ptr){
    BlockMT* current = memoryArea->blockList;
    while(current != NULL){
        if(current->dataPtr == ptr){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void customMTFree(void* ptr, int threadId){
    if(ptr == NULL){
        printf("<free error>: passed null pointer\n");
        return;
    }

    pthread_mutex_lock(&memoryAreaListMutex);
    if(memoryAreaList == NULL){
        printf("<free error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryAreaListMutex);
        return;
    }
    MemoryArea* memoryArea = findMemoryArea(ptr);
    if(memoryArea == NULL){
        printf("<free error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryAreaListMutex);
        return;
    }
    pthread_mutex_lock(&memoryArea->mutex);
    pthread_mutex_unlock(&memoryAreaListMutex);

    BlockMT* block = findBlockMT(memoryArea, ptr);
    if(block == NULL){
        printf("<free error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryArea->mutex);
        return;
    }
    if(block->free){
        printf("<free error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryArea->mutex);
        return;
    }
    block->free = true;

    // 1) Coalesce with NEXT if free
    if(block->next != NULL && block->next->free){
        BlockMT* nextBlock = block->next;
        block->size = block->size + nextBlock->size;
        block->next = nextBlock->next;
        if(block->next != NULL){
            block->next->prev = block;
        }
        pthread_mutex_lock(&heapSizeModificationMutex);
        customFree(nextBlock);
        pthread_mutex_unlock(&heapSizeModificationMutex);
    }
    // 2) Coalesce with PREV if free
    if(block->prev != NULL && block->prev->free){
        BlockMT* prevBlock = block->prev;
        prevBlock->size = prevBlock->size + block->size;
        prevBlock->next = block->next;
        if(prevBlock->next != NULL){
            prevBlock->next->prev = prevBlock;
        }
        pthread_mutex_lock(&heapSizeModificationMutex);
        customFree(block);
        pthread_mutex_unlock(&heapSizeModificationMutex);
    }
    pthread_mutex_unlock(&memoryArea->mutex);
}

void* customMTCalloc(size_t nmemb, size_t size, int threadId){
    void* ptr = customMTMalloc(nmemb * size, threadId);
    if(ptr == NULL){
        return NULL;
    }
    memset(ptr, 0, nmemb * size);
    return ptr;
}

void* customMTRealloc(void* ptr, size_t size, int threadId){
    if(ptr == NULL){
        return customMTMalloc(size, threadId);
    }

    pthread_mutex_lock(&memoryAreaListMutex);
    if(memoryAreaList == NULL){
        printf("<realloc error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryAreaListMutex);
        return NULL;
    }
    MemoryArea* memoryArea = findMemoryArea(ptr);
    if(memoryArea == NULL){
        printf("<realloc error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryAreaListMutex);
        return NULL;
    }
    pthread_mutex_lock(&memoryArea->mutex);

    BlockMT* block = findBlockMT(memoryArea, ptr);
    if(block == NULL){
        printf("<realloc error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryArea->mutex);
        pthread_mutex_unlock(&memoryAreaListMutex);
        return NULL;
    }
    if(block->free){
        printf("<realloc error>: passed non-heap pointer\n");
        pthread_mutex_unlock(&memoryArea->mutex);
        pthread_mutex_unlock(&memoryAreaListMutex);
        return NULL;
    }

    size_t newSize = ALIGN_TO_MULT_OF_4(size);
    // Realloc to the same size
    if(block->size == newSize){
        pthread_mutex_unlock(&memoryArea->mutex);
        pthread_mutex_unlock(&memoryAreaListMutex);
        return ptr;
    }

    // Realloc to larger size
    if(block->size < newSize){
        void* newPtr = customMTMalloc(newSize, threadId);
        if(newPtr == NULL){
            pthread_mutex_unlock(&memoryArea->mutex);
            pthread_mutex_unlock(&memoryAreaListMutex);
            return NULL;
        }
        memcpy(newPtr, ptr, block->size);
        customMTFree(ptr, threadId);
        pthread_mutex_unlock(&memoryArea->mutex);
        pthread_mutex_unlock(&memoryAreaListMutex);
        return newPtr;
    }

    // Realloc to smaller size; split the block into two blocks
    pthread_mutex_unlock(&memoryAreaListMutex);

    pthread_mutex_lock(&heapSizeModificationMutex);
    BlockMT* newBlock = (BlockMT*)customMalloc(sizeof(BlockMT));
    pthread_mutex_unlock(&heapSizeModificationMutex);
    if(newBlock == NULL){
        pthread_mutex_unlock(&memoryArea->mutex);
        return NULL;
    }
    block->size = newSize;
    newBlock->size = block->size - newSize;
    newBlock->dataPtr = (void*)((char*)block->dataPtr + newSize);

    newBlock->prev = block;
    newBlock->next = block->next;
    block->next = newBlock;
    if (newBlock->next != NULL){
        newBlock->next->prev = newBlock;
    }
    pthread_mutex_unlock(&memoryArea->mutex);
    return ptr;
}