#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)


int main()
{
	int fd, value, i, N, n;
    char bytes[4], ans[4];

    printf("Starting LKM testing.....\n");

    fd = open("/proc/partb_1_17CS30011_17CS30026", O_RDWR | O_APPEND);
    if(fd < 0) {
        printf("Failed to open the file\n");
    	return 1;
    }
    
    if(write(fd, "\xFF\x04", 2) < 0){
    	printf("Unable to write type and size");
    }
    
	N = 4;
    printf("Enter the Values to send\n");
    for(i = 0; i < N; i++) {
		n = i + 2048;
		
		bytes[0] = (n >> 24) & 0xFF;
		bytes[1] = (n >> 16) & 0xFF;
		bytes[2] = (n >> 8) & 0xFF;
		bytes[3] = n & 0xFF;
		
	    if(write(fd, bytes, sizeof(bytes)) < 0) {
	    	printf("Unable to write %d\n", n);
	    	break;
	    }
    }
    
    if(read(fd, ans, sizeof(ans)) < 0){
    	printf("Unable to read data");
    }
    
    printf("Min element: %s", ans);
    printf("\nExiting......\n");
    close(fd);

    return 0;
}
