#include "disk.h"

bool check_valid_blockptr(disk *diskptr, int blocknr){
    if(diskptr == NULL){
        return ERR;
    }

    return (blocknr >= 0 && blocknr < diskptr->blocks) == SUCC;
}

int read_block(disk *diskptr, int blocknr, void *block_data){
    if(check_valid_blockptr(diskptr, blocknr) == ERR){
        return ERR;
    }

    memcpy(block_data, diskptr->block_arr[blocknr], BLOCKSIZE);
    diskptr->reads++;

    return SUCC;
}