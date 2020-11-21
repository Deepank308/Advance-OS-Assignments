#include "sfs.h"
// #include "disk.h"
#include <assert.h>

int main()
{
    uint32_t nbytes = 409624;
    disk *diskptr = create_disk(nbytes);
    assert(diskptr != NULL);
    // disk_stat(diskptr);

    assert(format(diskptr) >= 0);
    // disk_stat(diskptr);

    assert(mount(diskptr) >= 0);
    // disk_stat(diskptr);

    int inum = create_file();
    disk_stat(diskptr);

    assert(inum == 1);

    int off = 0, i;
    int sz = 6 * BLOCKSIZE + BLOCKSIZE / 2;
    // int sz = 10;
    char data[sz];

    for (i = 0; i < sz; i++)
    	data[i] = (char)('0' + i%10);

    // int ret = write_i(inum, data, off, 0);
    // // disk_stat(diskptr);
    // assert(ret == off);

    int ret = write_i(inum, data, sz, off);
    disk_stat(diskptr);
    assert(ret == sz);

    char *data2 = (char *)malloc(sz * sizeof(char));
    ret = read_i(inum, data2, sz, off);
    assert(ret == sz);
    disk_stat(diskptr);

    for (i = 0; i < sz; i++){
    	assert(data2[i] == data[i]);
    }

    assert(remove_file(inum) >= 0);
    disk_stat(diskptr);

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
    char file2[] = "/dir1/dir3/file2";
    char dir4[] =  "/dir1/dir3/file1/dir4";

    char *read_data =  (char *)malloc(2 * sz * sizeof(char));

    printf("CREATING DIR 1...\n");
    assert(create_dir(dir1) == 0 && printf("CREATE DIR 1\n"));

    printf("CREATING DIR 2...\n");
    assert(create_dir(dir2) == 0 && printf("CREATE DIR 2\n"));

    printf("CREATING DIR 3...\n");
    assert(create_dir(dir3) == 0 && printf("CREATE DIR 3\n"));

    printf("WRITING FILE 2...\n");
    assert((write_file(file2, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 1\n"));

    printf("WRITING FILE 1...\n");
    assert((write_file(file1, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 1\n"));

    printf("WRITING FILE 1...\n");
    assert((write_file(file1, data, sizeof(data), sizeof(data)) >= 0) && printf("WRITE FILE 1\n"));

    printf("READING FILE 1...\n");
    assert((read_file(file1, read_data, 2*sizeof(data), 0) >= 0) && printf("READ FILE 1\n"));
    printf("FILE 1 DATA READ: %s\n", read_data);
    
    printf("REMOVING DIR 2...\n");
    assert(remove_dir(dir2) == 0 && printf("REMOVE DIR 2\n"));
    
    // return 0;
    printf("REMOVING DIR 1...\n");
    assert(remove_dir(dir1) == 0 && printf("REMOVE DIR 1\n"));

    // printf("REMOVING DIR 3...\n");
    // assert(remove_dir(dir3) == 0 && printf("REMOVE DIR 3\n"));

    // printf("WRITING FILE 1...\n");
    // assert((write_file(file1, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 1\n"));

    free_disk(diskptr);
    printf("TESTS SUCCESSFUL\n");
}
