#include "sfs.h"
// #include "disk.h"
#include <assert.h>

int main()
{
    uint32_t nbytes = 409600;
    disk *diskptr = create_disk(nbytes);
    assert(diskptr != NULL);

    assert(format(diskptr) >= 0);
    assert(mount(diskptr) >= 0);

    int inum = create_file();
    assert(inum == 1);

    int off = 0, i;
    char data[10];
    for (i = 0; i < 10; i++)
    	data[i] = (char)('0' + i);

    int ret = write_i(inum, data, 10, off);
    assert(ret == 10);

    char *data2 = (char *)malloc(10 * sizeof(char));
    ret = read_i(inum, data2, 10, off);
    assert(ret == 10);

    for (i = 0; i < 10; i++){
    	assert(data2[i] == data[i]);
    }   

    assert(remove_file(inum) >= 0);

    /*
    Directory structure
    
    /
    /dir1
    /dir2

    /dir1/dir3/file1

    /dir2/file2

    */

    char dir1[] = "/dir1";
    char dir2[] = "/dir2";
    char dir3[] = "/dir1/dir3";
    char file1[] = "/dir1/dir3/file1";
    char file2[] = "/dir2/file2";
    char dir4[] =  "/dir1/dir3/file1/dir4";

    char read_data[100]= "\n";

    printf("CREATING DIR 1...\n");
    assert(create_dir(dir1) == 0 && printf("CREATE DIR 1\n"));

    printf("CREATING DIR 2...\n");
    assert(create_dir(dir2) == 0 && printf("CREATE DIR 2\n"));

    printf("WRITING FILE 2...\n");
    assert((write_file(file2, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 1\n"));

    printf("CREATING DIR 3...\n");
    assert(create_dir(dir3) == 0 && printf("CREATE DIR 3\n"));

    printf("CREATING DIR 4...\n");
    assert(create_dir(file1) == 0 && printf("CREATE DIR 4\n"));

    printf("WRITING FILE 1...\n");
    assert((write_file(file1, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 1\n"));

    printf("WRITING FILE 1...\n");
    assert((write_file(file1, data, sizeof(data), sizeof(data)) >= 0) && printf("WRITE FILE 1\n"));

    printf("READING FILE 1...\n");
    assert((read_file(file1, read_data, 2*sizeof(data), 0) >= 0) && printf("READ FILE 1\n"));
    printf("FILE 1 DATA READ: %s\n", read_data);
    
    
    printf("REMOVING DIR 2...\n");
    assert(remove_dir(dir2) == 0 && printf("REMOVE DIR 2\n"));
    
    printf("REMOVING DIR 1...\n");
    assert(remove_dir(dir1) == 0 && printf("REMOVE DIR 1\n"));

    // printf("REMOVING DIR 3...\n");
    // assert(remove_dir(dir3) == 0 && printf("REMOVE DIR 3\n"));

    // printf("WRITING FILE 1...\n");
    // assert((write_file(file1, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 1\n"));

    free_disk(diskptr);
    printf("TESTS SUCCESSFUL\n");
}
