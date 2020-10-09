/*
Deepank Agrawal 17CS30011
Praagy Rastogi 17CS30026
Kirti Agarwal 20CS60R14

LKM built in kernel version 5.4.0
*/

#include "heap.h"

int32_t create_heap(heap** lkm_hp_p, int32_t type, int32_t size)
{
    if(size < 1 || size > HEAP_MXSIZE) return -1;

    heap *hp = (heap *) vmalloc(sizeof(heap));
    
    hp->type=type;
    hp->capacity=size;
    hp->count=0;
    hp->arr = (int32_t *) vmalloc(size*sizeof(int32_t));
    hp->last_inserted = '\0';
    
    *lkm_hp_p = hp;
    
    return 0;
}


void destroy_heap(heap* hp)
{
    vfree(hp->arr);
    vfree(hp);
}


int32_t insert(heap* hp, int32_t elem)
{
    if(hp->count >= hp->capacity) return -1;

    hp->arr[hp->count] = elem;
    pushup(hp, hp->count);
    hp->count++;
    hp->last_inserted = elem;

    return 0;
}


void pushup(heap* hp, int32_t idx)
{
    while(idx>0)
    {
        int32_t parent = (idx-1)/2;
        if(compare(hp, hp->arr[idx], hp->arr[parent]))
        {
            int32_t temp=hp->arr[idx];
            hp->arr[idx] = hp->arr[parent];
            hp->arr[parent] = temp;
            idx=parent;
        }
        else break;
    }
}


int32_t get_root(heap *hp, int32_t *top){
    if(hp->count == 0){
    	*top = '\0';
    	return 0;
    }

    *top = hp->arr[0];
    return 0;
}


int32_t extract(heap* hp, int32_t *top)
{
    if(hp->count==0) return -1;

    *top = hp->arr[0];
    
    hp->arr[0]=hp->arr[hp->count - 1];
    hp->count--;
    
    heapify(hp, 0);

    return 0;
}


int32_t compare(heap* hp, int32_t x, int32_t y)
{
    if(!hp->type) return x<=y;
    else return x>=y;
}


void heapify(heap* hp, int32_t idx)
{
    int32_t l = 2*idx+1;
    int32_t r = 2*idx+2;
    int32_t best = idx;
    if(l<hp->count && compare(hp, hp->arr[l], hp->arr[idx])) 
        best=l;
    
    if(r<hp->count && compare(hp, hp->arr[r], hp->arr[best]))
        best=r;
    
    if(best!=idx)
    {
        int32_t temp=hp->arr[idx];
        hp->arr[idx]=hp->arr[best];
        hp->arr[best]=temp;
        heapify(hp,best);
    }
}
