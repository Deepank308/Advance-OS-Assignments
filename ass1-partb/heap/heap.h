#ifndef __LKM_HEAP__
#define __LKM_HEAP__

#include<linux/vmalloc.h>
#define HEAP_MXSIZE 100

typedef struct heap
{
    int32_t type; // Max 1, Min 0
    int32_t capacity; 
    int32_t count;
    int32_t *arr;
    int32_t last_inserted;
}heap;


// allocates memory, initializes heap and returns pointer to it
int32_t create_heap(heap** hp, int32_t type, int32_t size);

// frees heap struct and heap array
void destroy_heap(heap* hp);

// insert element into heap, size increases by 1
int32_t insert(heap* hp, int32_t elem);

// fills the top element in the heap
int32_t get_root(heap *hp, int32_t *top);

// extracts top element from heap decreasing heap size by 1
int32_t extract(heap* hp, int32_t *top);

// returns 1 if x is supposed to get extracted before y
int32_t compare(heap* hp, int32_t x, int32_t y);

// maintains heap property (push down)
void heapify(heap* hp, int32_t idx);

// life element up to maintain heap property
void pushup(heap* hp, int32_t idx);

#endif
