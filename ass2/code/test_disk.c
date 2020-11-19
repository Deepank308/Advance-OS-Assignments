//! DO NOT SUBMIT THIS FILE

#include "disk.h"
#include <assert.h>

int main()
{
    uint32_t nbytes = 409624;
    disk *diskptr = create_disk(nbytes);
    assert(diskptr != NULL);

    assert(diskptr->size == nbytes);
    assert(diskptr->reads == 0 && diskptr->writes == 0 && diskptr->blocks == 100);

    char buff[BLOCKSIZE];
    assert(read_block(diskptr, 98, buff) == 0);
    assert(diskptr->reads == 1 && diskptr->writes == 0 && diskptr->blocks == 100);

    char buff2[BLOCKSIZE];
    int i;
    for (i = 0; i < BLOCKSIZE - 1; i++)
        buff2[i] = (char)((i%26) + 'a');
    buff2[BLOCKSIZE - 1] = '\0';

    assert(write_block(diskptr, 98, buff2) == 0);

    assert(diskptr->size == nbytes);
    assert(diskptr->reads == 1 && diskptr->writes == 1 && diskptr->blocks == 100);
    
    assert(read_block(diskptr, 98, buff) == 0);
    
    assert(diskptr->size == nbytes);
    assert(diskptr->reads == 2 && diskptr->writes == 1 && diskptr->blocks == 100);
    printf("%s", buff);
   
    assert(strcmp(buff, buff2) == 0);
    assert(free_disk(diskptr) == 0);

    printf("Test disk successfull");
}

