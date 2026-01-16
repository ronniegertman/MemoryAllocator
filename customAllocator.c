#include "customAllocator.h"
#include <stdbool.h>
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