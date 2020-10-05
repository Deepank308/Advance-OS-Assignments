#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

int32_t val[10] = /*{9, 3, 6, 4, 2, 5, 2, 12, 15, 7}; */ {999, 30034, 6, 4, -199999, 5, -2, 120003, 15, 7};
int32_t minsorted[10] = /* {2, 2, 3, 4, 5, 6, 7, 9, 12, 15}; */ {-199999, -2, 4, 5, 6, 7, 15, 999, 30034, 120003};
int32_t maxsorted[10] = /* {15, 12, 9, 7, 6, 5, 4, 3, 2, 2}; */ {120003, 30034, 999, 15, 7, 6, 5, 4, -2, -199999};
int iter = 3;


int run_min_heap(char *procfile){

	// Initialize min heap =============================================
	// printf("==== Initializing Min Heap ====\n");
	int fd = open(procfile, O_RDWR);
	if(fd < 0){
	    perror("Could not open flie /proc/ass1");
	    return 0;
	}

	// min heap
	char buf[2];
	int32_t tmp;
	
	buf[0] = 0xFF;
	// size of the heap
	buf[1] = 100;

	int result = write(fd, buf, 2);
	if(result < 0){
	    perror("Write failed");
	    close(fd);
	    return 0;
	}
	// printf("Written %d bytes\n", result);

	// Test Min Heap ====================================================
	// printf("======= Test Min Heap =========\n");
	while(iter--){
		// Insert --------------------------------------------------------
		for (int i = 0; i < 10; i++)
		{
			// printf("Inserting %d\n", val[i]);
			result = write(fd, &val[i], sizeof(int32_t));
			if(result < 0){
			    perror("ERROR! Write failed");
			    close(fd);
			    return 0;
			}
			// printf("Written %d bytes\n", result);
		}

		// Verify ---------------------------------------------------------
		for (int i = 0; i < 10; i++)
		{
			// printf("Extracting..\n");
			result = read(fd, (void *)(&tmp), sizeof(int32_t));
			if(result < 0){
			    perror(RED "ERROR! Read failed\n" RESET);
			    close(fd);
			    return 0;
			}
			// printf("Extracted: %d\n", (int)tmp);
			if(tmp == minsorted[i]){
			    printf(GRN "Results Matched\n" RESET);
			}
			else {
			    printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)minsorted[i], (int)tmp);
			}
		}
	}

	close(fd);
	return 0;
}


int run_max_heap(char *procfile){
	
	// Initialize max heap =============================================
	// printf("==== Initializing Max Heap ====\n");
	int fd = open(procfile, O_RDWR);
	if(fd < 0){
	    perror("Could not open flie /proc/ass1");
	    return 0;
	}
	
	char buf[2];
	int32_t tmp;
	
	// max heap
	buf[0] = 0xF0;
	// size of the heap
	buf[1] = 20;

	int result = write(fd, buf, 2);
	if(result < 0){
	    perror("Write failed");
	    close(fd);
	    return 0;
	}
	// printf("Written %d bytes :0\n", result);
	
	// Test Max Heap ====================================================
	// printf("======= Test Max Heap =========\n");
	while(iter--){
		// Insert --------------------------------------------------------
		for (int i = 0; i < 10; i++)
		{
			// printf("Inserting 0: %d\n", val[i]);
			result = write(fd, &val[i], sizeof(int32_t));
			if(result < 0){
			    perror("ERROR! Write failed 0");
			    close(fd);
			    return 0;
			}
			// printf("Written %d bytes\n", result);
		}
		
		// Verify ---------------------------------------------------------
		for (int i = 0; i < 10; i++)
		{
			// printf("Extracting..\n");
			result = read(fd, (void *)(&tmp), sizeof(int32_t));
			if(result < 0){
			    perror("ERROR! Read failed 0");
			    close(fd);
			    return 0;
			}
			// printf("Extracted 0: %d\n", (int)tmp);
			if(tmp == maxsorted[i]){
			    printf(GRN "Results Matched 0\n" RESET);
			}
			else {
			    printf(RED "ERROR! Results Do Not Match 0. Expected %d, Found %d\n" RESET, (int)maxsorted[i], (int)tmp);
			}
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
	int i = 10;
	while(i--){
		if((pid = fork()) == 0){
			// parent process
			continue;
		}
		else{
			// childs
			if(i&1) while(!run_max_heap(procfile));
			else while(!run_min_heap(procfile));
			exit(0);
		}
	}
	int n = 10;
	while(n--){
		int chpid = wait(NULL);
		printf("Child Process terminated: %d\n", chpid);
	}

   return 0;
}
