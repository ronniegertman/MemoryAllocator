#ifndef __CUSTOM_ALLOCATOR__
#define __CUSTOM_ALLOCATOR__

/*=============================================================================
* do no edit lines below!
=============================================================================*/
#include <stddef.h> //for size_t
#include <stdbool.h> //for bool
#include <pthread.h> //for pthread_mutex_t

//Part A - single thread memory allocator
void* customMalloc(size_t size);
void customFree(void* ptr);
void* customCalloc(size_t nmemb, size_t size);
void* customRealloc(void* ptr, size_t size);
void* bestFit(size_t size);

//Part B - multi thread memory allocator
void* customMTMalloc(size_t size, int threadNumber);
void customMTFree(void* ptr);
void* customMTCalloc(size_t nmemb, size_t size);
void* customMTRealloc(void* ptr, size_t size);

// Part B - helper functions for multi thread memory allocator
void heapCreate();
void heapKill();

/*=============================================================================
* do no edit lines above!
=============================================================================*/

/*=============================================================================
* defines
=============================================================================*/
#define SBRK_FAIL (void*)(-1)
#define ALIGN_TO_MULT_OF_4(x) (((((x) - 1) >> 2) << 2) + 4)

/*=============================================================================
* Block
=============================================================================*/
//suggestion for block usage - feel free to change this
typedef struct Block
{
    size_t size; // in bytes
    struct Block* next;
    struct Block* prev;
    bool free;
} Block;
extern Block* blockList;

typedef struct BlockMT
{
    size_t size; // in bytes
    struct BlockMT* next;
    struct BlockMT* prev;
    bool free;
    void* dataPtr;
} BlockMT;

typedef struct MemoryArea
{
    size_t size;
    size_t freeMemory; // to delete
    pthread_mutex_t mutex;
    void* dataPtr;
    BlockMT* blockList;
    bool used; // to delete
    struct MemoryArea* next;
} MemoryArea;
extern MemoryArea* memoryAreaList;
extern MemoryArea* lastMemoryArea;
extern pthread_mutex_t memoryAreaListMutex;

#endif // CUSTOM_ALLOCATOR
