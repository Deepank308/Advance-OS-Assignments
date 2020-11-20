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
    assert(stat(inum) >= 0);

    int off = 0, i;
    char data[10];
    for (i = 0; i < 10; i++)
    	data[i] = (char)('0' + i);

    int ret = write_i(inum, data, 10, off);
    assert(ret == 10);

    char *data2 = (char *)malloc(10 * sizeof(char));
    ret = read_i(inum, data2, 10, off);
    assert(ret == 10);

    // for(int i = 0; i < 10; i++) printf("%c\n", data2[i]);
    for (i = 0; i < 10; i++){
        // printf("%c, %c\n", data[i], data2[i]);
    	assert(data2[i] == data[i]);
    }   

    assert(stat(inum) >= 0);

    assert(remove_file(inum) >= 0);

    char buf[6] = "/some";
    char buf2[6] = "/some";
    assert(create_dir(buf) == 0 && printf("Create Dir\n"));
   
    assert(write_file(buf, buf2, strlen(buf2), 0) == 0 && printf("Write File \n"));
    // return 0;
    
    assert(read_file(buf, buf2, strlen(buf2), 0) == 0 && printf("Read file \n"));
    assert(remove_dir(buf) == 0 && printf("Remove Dir \n"));

    free_disk(diskptr);
    printf("Tests successful\n");
}
