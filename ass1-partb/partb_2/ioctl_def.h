/*
Deepank Agrawal 17CS30011
Praagy Rastogi 17CS30026
Kirti Agarwal 20CS60R14

LKM built in kernel version 5.4.0
*/

#ifndef __PB2_IOCTL_DEF__
#define __PB2_IOCTL_DEF__ 

#define PB2_SET_TYPE    _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT      _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO    _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT     _IOR(0x10, 0x34, int32_t*)


typedef struct pb2_set_type_arguments {
    int32_t heap_size; // size of the heap
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
}pb2_set_type_arguments;


typedef struct obj_info {
    int32_t heap_size; // size of the heap
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
    int32_t root;      // value of the root node of the heap (null if size is 0).
    int32_t last_inserted; // value of the last element inserted in the heap.
}obj_info;


typedef struct result {
    int32_t result;    // value of min/max element extracted.
    int32_t heap_size; // size of the heap after extracting.
}result;


#endif
