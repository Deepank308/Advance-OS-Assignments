/*
Deepank Agrawal 17CS30011
Praagy Rastogi 17CS30026
Kirti Agarwal 20CS60R14
*/

#include <assert.h>

#include "sfs.h"

int main()
{
    printf("\n----------------- Testing Part B -----------------\n");
    uint32_t nbytes = 409624;
    disk *diskptr = create_disk(nbytes);
    assert(diskptr != NULL && printf("Create Disk: SUCCESS\n"));
    disk_stat(diskptr);

    assert(format(diskptr) >= 0 && printf("\nFormat SFS: SUCCESS\n"));
    disk_stat(diskptr);

    assert(mount(diskptr) >= 0 && printf("\nMount SFS: SUCCESS\n"));
    disk_stat(diskptr);

    int inum = create_file();
    assert(inum == 1 && printf("\nCreate File: SUCCESS\n"));
    disk_stat(diskptr);

    int off = 0, i;
    int sz = 6 * BLOCKSIZE + BLOCKSIZE / 2;
    char data[sz];
    for (i = 0; i < sz; i++)
        data[i] = (char)('0' + i%10);

    int ret = write_i(inum, data, sz, off);
    assert(ret == sz && printf("\nWrite inode %d: SUCCESS\n", inum));
    disk_stat(diskptr);

    char *data2 = (char *)malloc(sz * sizeof(char));
    ret = read_i(inum, data2, sz, off);
    assert(ret == sz && printf("\nRead inode %d: SUCCESS\n", inum));
    disk_stat(diskptr);
    stat(inum);

    for (i = 0; i < sz; i++){
        assert(data2[i] == data[i]);
    }

    assert(remove_file(inum) >= 0 && printf("\nRemove inode %d: SUCCESS\n", inum));
    disk_stat(diskptr);
    stat(inum);
    printf("\n-------------------------------------------------\n");

    printf("\n----------------- Testing Part C ----------------\n");
    printf("Directory structure:\nroot:\n/\n\nroot contents:\ndir1\ndir2\n\ndir1/ contents:\ndir3/\n\ndir2/ contents:\nfile2\n\ndir3/ contents:\nfile1\n\n");

    char dir1[] = "/dir1";
    char dir2[] = "/dir2";
    char dir3[] = "/dir1/dir3";
    char file1[] = "/dir1/dir3/file1";
    char file2[] = "/dir2/file2";

    char *read_data =  (char *)malloc(2 * sz * sizeof(char));

    assert(create_dir(dir1) == 0 && printf("CREATE DIR 1: SUCCESS\n"));
    stat(0);

    assert(create_dir(dir2) == 0 && printf("CREATE DIR 2: SUCCESS\n"));
    stat(0);

    assert(create_dir(dir3) == 0 && printf("CREATE DIR 3: SUCCESS\n"));

    assert((write_file(file2, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 2 at offset 0: SUCCESS\n"));

    assert((write_file(file1, data, sizeof(data), 0) >= 0) && printf("WRITE FILE 1 at offset 0: SUCCESS\n"));

    assert((write_file(file1, data, sizeof(data), sizeof(data)) >= 0) && printf("WRITE FILE 1 at offset %ld: SUCCESS\n", sizeof(data)));

    assert((read_file(file1, read_data, 2*sizeof(data), 0) >= 0) && printf("READ FILE 1: SUCCESS\n"));

    for (i = 0; i < sz; i++){
        assert(read_data[i] == data[i]);
        assert(read_data[i + sizeof(data)] == data[i]);
    }

    assert(remove_dir(dir2) == 0 && printf("REMOVE DIR 2: SUCCESS\n"));

    assert(remove_dir(dir3) == 0 && printf("REMOVE DIR 3: SUCCESS\n"));

    assert(remove_dir(dir1) == 0 && printf("REMOVE DIR 1: SUCCESS\n"));

    free(read_data);
    free(data2);

    free_disk(diskptr);
    printf("\n-------------------------------------------------\n");

    printf("\nTESTS SUCCESSFUL\n");
}
