#define _GNU_SOURCE
#include <string.h>

#include "customAllocator.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void test_single_thread() {
    // printf("Testing single thread\n");
    heapCreate();

    void* ptr = customMTMalloc(1024, 1);
    printf("Allocated %p\n", ptr);
    // print block list
    customMTFree(ptr, 1);
    printf("Freed %p\n", ptr);
    heapKill();
}

void test_single_thread_realloc() {
    heapCreate();

    // Malloc
    void* ptr = customMTMalloc(1024, 1);
    printf("Allocated %p\n", ptr);
    memset(ptr, 0, 1024);
    
    // Realloc
    void* ptr2 = customMTRealloc(ptr, 1024, 1);
    printf("Reallocated %p\n", ptr2);
    memset(ptr2, 1, 2048);

    // Free
    customMTFree(ptr2, 1);
    printf("Freed %p\n", ptr2);

    heapKill();
}


typedef struct {
  pthread_barrier_t *start_barrier;
  int threadNumber;
  // put other args here
} worker_arg_t;

void *worker(void *p) {
  worker_arg_t *a = (worker_arg_t *)p;

  // Thread is created, now wait until everyone is ready.
  pthread_barrier_wait(a->start_barrier);

  // ---- real work starts here ----
  // your code that uses your allocator-sensitive logic

  printf("Thread %d allocating...\n", a->threadNumber);
  void* ptr = customMTCalloc(4, 1024, a->threadNumber);
  printf("Thread %d allocated %p\n", a->threadNumber, ptr);
  memset(ptr, 0, 4096);
  customMTFree(ptr, a->threadNumber);
  printf("Thread %d freed %p\n", a->threadNumber, ptr);

  return NULL;
}


// worker that tests customMTMalloc, customMTRealloc, customMTFree
void *worker_realloc(void *p) {
  static const size_t FIRST_SIZE = 512;
  static const size_t SECOND_SIZE = 1024;

  worker_arg_t *a = (worker_arg_t *)p;
  pthread_barrier_wait(a->start_barrier);

  // Malloc
  printf("Thread %d allocating...\n", a->threadNumber);
  void* ptr = customMTMalloc(FIRST_SIZE, a->threadNumber);
  printf("Thread %d allocated %p\n", a->threadNumber, ptr);
  memset(ptr, 0, FIRST_SIZE);

  // // Realloc
  void* ptr2 = customMTRealloc(ptr, SECOND_SIZE, a->threadNumber);
  printf("Thread %d reallocated %p\n", a->threadNumber, ptr2);
  memset(ptr2, 1, SECOND_SIZE);

  // Free
  customMTFree(ptr2, a->threadNumber);
  printf("Thread %d freed %p\n", a->threadNumber, ptr2);

  return NULL;
}

int test_threads(void) {
  const int N = 200;
  pthread_t th[N];
  worker_arg_t args[N];

  pthread_barrier_t start_barrier;
  pthread_barrier_init(&start_barrier, NULL, N + 1); // +1 for main

  // Optional: "warm up" anything that might allocate *before* you start.
  // e.g., call functions you know will be used later, do a dummy malloc/free, etc.

  for (int i = 0; i < N; i++) {
    args[i].start_barrier = &start_barrier;
    args[i].threadNumber = i + 1;
    if (pthread_create(&th[i], NULL, worker_realloc, &args[i]) != 0) {
      perror("pthread_create");
      exit(1);
    }
  }

  // At this point, threads exist but are stuck at the barrier.
  // If pthread/libc needed to allocate during thread creation, it already happened.

  // If you have a “switch” to enter your allocator-sensitive phase, do it now.
  heapCreate();

  // Release all workers to start real work:
  pthread_barrier_wait(&start_barrier);

  for (int i = 0; i < N; i++) pthread_join(th[i], NULL);
  pthread_barrier_destroy(&start_barrier);

  heapKill();
  return 0;
}


int main(void) {
//   test_single_thread();
  test_threads();
  // test_single_thread_realloc();
  return 0;
}