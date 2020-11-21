/*
Deepank Agrawal 17CS30011
Praagy Rastogi 17CS30026
Kirti Agarwal 20CS60R14
*/

#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>

#define ERR -1
#define SUCC 0

#define VALID 1
#define INVALID 0

const static int BLOCKSIZE = 4*1024;

typedef struct disk {
    uint32_t size; // size of the disk
    uint32_t blocks; // number of usable blocks (except stat block)
    uint32_t reads; // number of block reads performed
    uint32_t writes; // number of block writes performed
    char **block_arr; // array (of size blocks) of pointers to 4KB blocks
} disk;


void disk_stat(disk *diskptr);

disk *create_disk(int nbytes);

int read_block(disk *diskptr, int blocknr, void *block_data);

int write_block(disk *diskptr, int blocknr, void *block_data);

int free_disk(disk *diskptr);
