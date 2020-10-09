#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#define RED   "\x1B[31m"
// #define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

#define PB2_SET_TYPE    _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT      _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO    _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT     _IOR(0x10, 0x34, int32_t*)


struct pb2_set_type_arguments { 
    int32_t heap_size; // size of the heap
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
};
struct obj_info {
    int32_t heap_size; // current number of elements in heap (current size of the heap)
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
    int32_t root; // value of the root node of the heap (null if size is 0).
    int32_t last_inserted; // value of the last element inserted in the heap.
};

struct result {
    int32_t result;         // value of min/max element extracted.
    int32_t heap_size;     // current size of the heap after extracting.
};


int32_t tmp;
int32_t val[10] = {9, 3, 6, 4, 2, 5, 2, 12, 15, 7};
int32_t minsorted[10] = {2, 2, 3, 4, 5, 6, 7, 9, 12, 15};
int32_t maxsorted[10] = {15, 12, 9, 7, 6, 5, 4, 3, 2, 2};


struct pb2_set_type_arguments mypb2_set_type_arguments;
struct obj_info myobj_info;
struct result myresult;


int run_min_heap(char *procfile){
    int fd = open(procfile, O_RDWR) ;

    if(fd < 0){
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    
    // Initialize min heap =============================================
    printf("==== Initializing Min Heap ====\n");
    char buf[2];
    // min heap
    mypb2_set_type_arguments.heap_type = 0;
    // size of the heap
    mypb2_set_type_arguments.heap_size = 20;

    int result = ioctl(fd, PB2_SET_TYPE, &mypb2_set_type_arguments);

    if(result < 0){
        perror("Initialization failed");
        close(fd);
        return 0;
    }

    // Check info
    result = ioctl(fd, PB2_GET_INFO, &myobj_info);
    if(myobj_info.heap_size == 0){
        // printf(GRN "Size Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Size Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_size);
    }

    if(myobj_info.heap_type == 0){
        // printf(GRN "Type Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Type Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_type);
    }

    if(myobj_info.root == NULL){
        // printf(GRN "ROOT Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! ROOT Do Not Match. Expected %d, Found %d\n" RESET, (int)NULL, (int)myobj_info.root);
    }

    if(myobj_info.last_inserted == NULL){
        // printf(GRN "last_inserted Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! last_inserted Do Not Match. Expected %d, Found %d\n" RESET, (int)NULL, (int)myobj_info.last_inserted);
    }

    // Test Min Heap ====================================================
    printf("======= Test Min Heap =========\n");

    // Insert --------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Inserting %d\n", val[i]);
        result = ioctl(fd, PB2_INSERT, &val[i]);
        if(result < 0){
            perror("ERROR! Insert failed");
            close(fd);
            return 0;
        }
    }

    // Check info
    result = ioctl(fd, PB2_GET_INFO, &myobj_info);
    if(myobj_info.heap_size == 10){
        // printf(GRN "Size Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Size Do Not Match. Expected %d, Found %d\n" RESET, 10, (int)myobj_info.heap_size);
    }

    if(myobj_info.heap_type == 0){
        // printf(GRN "Type Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Type Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_type);
    }

    if(myobj_info.root == 2){
        // printf(GRN "ROOT Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! ROOT Do Not Match. Expected %d, Found %d\n" RESET, (int)2, (int)myobj_info.root);
    }

    if(myobj_info.last_inserted == 7){
        // printf(GRN "last_inserted Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! last_inserted Do Not Match. Expected %d, Found %d\n" RESET, (int)7, (int)myobj_info.last_inserted);
    }
    
    // Verify ---------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Extracting..\n");
        result = ioctl(fd, PB2_EXTRACT, &myresult);
        if(result < 0){
            perror(RED "ERROR! Read failed" RESET);
            close(fd);
            return 0;
        }
        printf("Extracted: %d\n", (int)myresult.result);
        if(myresult.result == minsorted[i]){
            // printf(GRN "Results Matched\n" RESET);
        }
        else {
            printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d" RESET, (int)minsorted[i], (int)myresult.result);
        }
    }

    close(fd);
    return 0;
}


int run_max_heap(char *procfile){
    int fd = open(procfile, O_RDWR) ;

    if(fd < 0){
        perror("Max Heap Could not open flie /proc/ass1");
        return 0;
    }
    
    // Initialize min heap =============================================
    printf("==== Initializing Max Heap ====\n");
    char buf[2];
    // max heap
    mypb2_set_type_arguments.heap_type = 1;
    // size of the heap
    mypb2_set_type_arguments.heap_size = 20;

    int result = ioctl(fd, PB2_SET_TYPE, &mypb2_set_type_arguments);

    if(result < 0){
        perror("Max Heap Initialization failed");
        close(fd);
        return 0;
    }

    // Check info
    result = ioctl(fd, PB2_GET_INFO, &myobj_info);
    if(myobj_info.heap_size == 0){
        // printf(GRN "Max Heap Size Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! Size Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_size);
    }

    if(myobj_info.heap_type == 1){
        // printf(GRN "Max Heap Type Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! Type Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_type);
    }

    if(myobj_info.root == NULL){
        // printf(GRN "Max Heap ROOT Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! ROOT Do Not Match. Expected %d, Found %d\n" RESET, (int)NULL, (int)myobj_info.root);
    }

    if(myobj_info.last_inserted == NULL){
        // printf(GRN "Max Heap last_inserted Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! last_inserted Do Not Match. Expected %d, Found %d\n" RESET, (int)NULL, (int)myobj_info.last_inserted);
    }

    // Test Max Heap ====================================================
    printf("======= Test MAX Heap =========\n");

    // Insert --------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Max Heap Inserting %d\n", val[i]);
        result = ioctl(fd, PB2_INSERT, &val[i]);
        if(result < 0){
            perror("Max Heap ERROR! Insert failed");
            close(fd);
            return 0;
        }
    }

    // Check info
    result = ioctl(fd, PB2_GET_INFO, &myobj_info);
    if(myobj_info.heap_size == 10){
        // printf(GRN "Max Heap Size Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! Size Do Not Match. Expected %d, Found %d\n" RESET, 10, (int)myobj_info.heap_size);
    }

    if(myobj_info.heap_type == 1){
        // printf(GRN "Max Heap Type Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! Type Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_type);
    }

    if(myobj_info.root == 15){
        // printf(GRN "Max Heap ROOT Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! ROOT Do Not Match. Expected %d, Found %d\n" RESET, (int)2, (int)myobj_info.root);
    }

    if(myobj_info.last_inserted == 7){
        // printf(GRN "Max Heap last_inserted Matched\n" RESET);
    }
    else {
        printf(RED "Max Heap ERROR! last_inserted Do Not Match. Expected %d, Found %d\n" RESET, (int)7, (int)myobj_info.last_inserted);
    }
    
    // Verify ---------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Max Heap Extracting..\n");
        result = ioctl(fd, PB2_EXTRACT, &myresult);
        if(result < 0){
            perror(RED "Max Heap ERROR! Read failed\n" RESET);
            close(fd);
            return 0;
        }
        printf("Max Heap Extracted: %d\n", (int)myresult.result);
        if(myresult.result == maxsorted[i]){
            // printf(GRN "Max Heap Results Matched\n" RESET);
        }
        else {
            printf(RED "Max Heap ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)maxsorted[i], (int)myresult.result);
        }
    }

    close(fd);
    return 0;
}


int main(int argc, char *argv[]){
    if( argc != 2 ) {
        printf("Please provide your LKM proc file name as an argument.\nExample:\n./test_ass1 partb_1_<roll no>\n");
        return -1;
    }

    char procfile[100] = "/proc/";
    strcat(procfile, argv[1]);

    int pid = -1;
	int i = 20;
	while(i--){
		if((pid = fork()) == 0){
			// parent process
			continue;
		}
		else{
			// childs
			if(i&1) while(!run_min_heap(procfile));
			else while(!run_max_heap(procfile));
			exit(0);
		}
	}

	int n = 20;
	while(n--){
		int chpid = wait(NULL);
		printf("Child Process terminated: %d\n", chpid);
	}

   return 0;
}
