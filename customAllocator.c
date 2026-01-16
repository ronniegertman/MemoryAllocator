#include "customAllocator.h"
#include <stdbool.h>
#include <stdio.h> //for printf
#include <stdint.h> //for intptr_t
#include <unistd.h> //for sbrk

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
    lastBlock = newBlock;
    return (void*)(lastBlock + 1);
}

static Block* findBlock(void* ptr){
    Block* current = blockList;
    while(current != NULL){
        if(current + 1 == ptr){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void customFree(void* ptr){
    if (ptr == NULL){
        printf("<free error>: passed null pointer\n");
        return;
    }
    void* heapStart = (void*)blockList;
    void* heapEnd = sbrk(0);
    if(ptr < heapStart || ptr >= heapEnd){
        printf("<free error>: passed non-heap pointer\n");
        return;
    }

    Block* block = findBlock(ptr);
    if(block == NULL){
        // This is a heap pointer, but not allocated by the allocator
        printf("<free error>: passed non-heap pointer\n");
        return;
    }

    if (block->free) {
        // This is a heap pointer, but already freed
        printf("<free error>: passed non-heap pointer\n");
        return;
    }

    block->free = true;

    if(block->next != NULL && block->next->free){
        if(block->next == lastBlock){
            lastBlock = block;
        }
        block->size += block->next->size + sizeof(Block); // new block size includes the header of the second block
        block->next = block->next->next;
    }
    if(block->prev != NULL && block->prev->free){
        if(block == lastBlock){
            lastBlock = block->prev;
        }
        block->prev->size += block->size + sizeof(Block); // new block size includes the header of the original block
        block->prev->next = block->next;
    }

    if(lastBlock->free){
        sbrk(-1 * (lastBlock->size + sizeof(Block))); // TODO: check if we can handle sbrk failed
        return;
    }
}