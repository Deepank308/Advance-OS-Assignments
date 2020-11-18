#include "disk.h"

bool check_valid_blockptr(disk *diskptr, int blocknr){
    if(diskptr == NULL){
        return INVALID;
    }

    if((blocknr < 0 || blocknr >= diskptr->blocks)){
        return INVALID;
    }
    
    return VALID;
}

disk *create_disk(int nbytes)
{
    // allocate memory for disk structure
    disk* diskptr = (disk*)malloc(sizeof(disk));

    // remaining bytes
    int rembytes = nbytes-sizeof(disk);
    if(rembytes<0) 
        return NULL;

    diskptr->size = nbytes;
    diskptr->reads=0;
    diskptr->writes=0;
    diskptr->blocks = rembytes/BLOCKSIZE;
    diskptr->block_arr = (char**)malloc(diskptr->blocks * BLOCKSIZE);

    if(diskptr->block_arr==NULL) // malloc unsuccessful
        return NULL;

    return diskptr;
}

int read_block(disk *diskptr, int blocknr, void *block_data){
    if(check_valid_blockptr(diskptr, blocknr) == INVALID){
        return ERR;
    }

    memcpy(block_data, diskptr->block_arr[blocknr], BLOCKSIZE);
    diskptr->reads++;

    return SUCC;
}

int free_disk(disk *diskptr)
{
    if(diskptr==NULL) return ERR;
    free(diskptr->block_arr);
    free(diskptr);
    return SUCC;
}

