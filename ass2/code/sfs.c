#include "disk.h"
#include "sfs.h"

#define BITS_PER_BYTES 8
#define PTRS_PER_BLOCK BLOCKSIZE / 4
#define INODE_PER_BLOCK BLOCKSIZE / 32 // 128

int format(disk *diskptr)
{
    if (diskptr == NULL)
        return ERR;

    int N = diskptr->blocks;
    int M = N - 1;
    int I = 0.1 * M;

    int num_inodes = I * INODE_PER_BLOCK;
    int len_inode_bitmaps = num_inodes;
    int IB = ceil(1.0 * num_inodes / (BITS_PER_BYTES * BLOCKSIZE));

    // remaining blocks for data blocks and data bit maps
    int R = M - I - IB;
    int len_data_bitmaps = R;
    int DBB = ceil(1.0 * len_data_bitmaps / (BITS_PER_BYTES * BLOCKSIZE));
    int DB = R - DBB;
    // NOTE: only consider the first DB number of bits of the data block bitmap.

    super_block SB;
    SB.magic_number = MAGIC;
    SB.blocks = M;
    SB.inode_blocks = I;
    SB.inodes = num_inodes;
    SB.inode_bitmap_block_idx = 1;
    SB.inode_block_idx = 1 + IB + DBB;
    SB.data_block_bitmap_idx = 1 + IB;
    SB.data_block_idx = 1 + IB + DBB + I;
    SB.data_blocks = DB;

    // write super block to disk
    if (write_block(diskptr, 0, &SB) == ERR)
    {
        return ERR;
    }

    // initialize bitmaps(both inodes and data bitmaps)
    void *ptr_zero = malloc(BLOCKSIZE);
    memset(ptr_zero, 0, BLOCKSIZE);
    for (int i = SB.inode_bitmap_block_idx; i < SB.inode_block_idx; i++)
    {
        if (write_block(diskptr, i, ptr_zero) == ERR)
        {
            return ERR;
        }
    }

    // initialize inodes
    inode *inodes_array = (inode *)malloc(INODE_PER_BLOCK * sizeof(inode));
    for (int i = 0; i < INODE_PER_BLOCK; i++)
    {
        inodes_array[i].valid = 0;
    }

    for (int i = SB.inode_block_idx; i < SB.data_block_idx; i++)
    {
        if (write_block(diskptr, i, inodes_array) == ERR)
        {
            return ERR;
        }
    }

    return SUCC;
}

void init_mounted_fs(super_block *sb, disk *diskptr)
{
    fs.sb = *sb;
    fs.diskptr = diskptr;

    // init the bmaps
    fs.len_inode_bmap = sb->inodes / 32;
    fs.len_data_bmap = sb->data_blocks / 32;

    fs.inode_bmap = (uint32_t *)malloc(fs.len_inode_bmap * sizeof(uint32_t));
    fs.data_bmap = (uint32_t *)malloc(fs.len_data_bmap * sizeof(uint32_t));

    memset(fs.inode_bmap, 0, fs.len_inode_bmap * sizeof(uint32_t));
    memset(fs.data_bmap, 0, fs.len_data_bmap * sizeof(uint32_t));

    fs.rblock = malloc(BLOCKSIZE);
}

int mount(disk *diskptr)
{
    if (diskptr == NULL || fs.diskptr != NULL)
    {
        return ERR;
    }

    super_block sb;
    if (read_block(diskptr, 0, &sb) == ERR)
    {
        return ERR;
    }

    if (sb.magic_number != MAGIC)
    {
        return ERR;
    }

    init_mounted_fs(&sb, diskptr);
    return SUCC;
}

void flip_bit_in_bmap(uint32_t *bmap, uint32_t bit_position, uint32_t offset_block_idx)
{
    int bmap_block_idx = bit_position / (8 * BLOCKSIZE);
    int bit_offset = bit_position % (BLOCKSIZE * 8);

    read_block(fs.diskptr, offset_block_idx + bmap_block_idx, fs.rblock);

    int *bmap_block = (uint32_t *)fs.rblock;
    int update_idx = bit_offset / 32;
    int bit = (31 - update_idx) % 32;
    bmap_block[update_idx] ^= (1 << bit);
    bmap[bit_position / 32] ^= (1 << (31 - (bit_position % 32)));

    write_block(fs.diskptr, offset_block_idx + bmap_block_idx, bmap_block);
}

int get_free_bmap_idx(uint32_t *bmap, uint32_t len_bmap, uint32_t bmap_offset)
{
    int idx = -1; // 0 based idx of the required bit from start of bmap
    for (int i = 0; i < len_bmap; i++)
    {
        int mask = bmap[i];
        int bit;
        for (bit = 31; bit >= 0; bit--)
        {
            if (!(mask & (1 << bit)))
            {
                // free bit, mark this set
                bmap[i] |= (1 << bit);
                break;
            }
        }
        if (bit != -1)
        {
            idx = i * 32 + (32 - bit) - 1;
            break;
        }
    }

    if (idx == -1)
        return ERR;

    // update bitmap
    flip_bit_in_bmap(idx, bmap_offset);

    return idx;
}

int get_inode_from_disk(int inode_idx, int *inode_block_idx, int *inode_num)
{
    *inode_block_idx = inode_idx / INODE_PER_BLOCK;
    *inode_num = inode_idx % INODE_PER_BLOCK;

    if (read_block(fs.diskptr, fs.sb.inode_block_idx + *inode_block_idx, fs.rblock) == ERR)
    {
        return ERR;
    }

    return SUCC;
}

int create_file()
{
    int inode_idx = get_free_bmap_idx(fs.inode_bmap, fs.len_inode_bmap, fs.sb.inode_bitmap_block_idx);
    if (inode_idx == ERR)
        return ERR;

    // update inode block
    int inode_block_idx, inode_num;
    if (get_inode_from_disk(inode_idx, &inode_block_idx, &inode_num) == ERR)
    {
        return ERR;
    }

    inode *new_ptr;
    new_ptr = (inode *)fs.rblock; //typecasting in inode* type;
    new_ptr += inode_num;         //used for incrementing the blockptr to get to specific inode
    new_ptr->size = 0;
    new_ptr->valid = VALID;

    if (write_block(fs.diskptr, inode_block_idx, fs.rblock) == ERR) //writing back updates block into disk
        return ERR;
}

bool check_valid_inode(int inumber)
{
    if (inumber < 0 || inumber > fs.sb.inodes)
        return INVALID;

    return VALID;
}

int remove_file(int inumber)
{
    if (check_valid_inode(inumber) == INVALID)
        return ERR;

    // set inode.valid=0
    int inode_block_idx, inode_num;
    if (get_inode_from_disk(inumber, &inode_block_idx, &inode_num) == ERR)
    {
        return ERR;
    }

    inode in = ((inode *)fs.rblock)[inode_num];
    if (in.valid == INVALID)
    {
        return ERR;
    }

    ((inode *)fs.rblock)[inode_num].valid = INVALID;
    write_block(fs.diskptr, fs.sb.inode_block_idx + inode_block_idx, fs.rblock);

    // update inode bmaps
    flip_bit_in_bmap(fs.inode_bmap, inumber, fs.sb.inode_bitmap_block_idx);

    // update data bmaps
    uint32_t cur_dblock = 0, num_dblocks = ceil(in.size / BLOCKSIZE);
    uint32_t indirect_ptrs[PTRS_PER_BLOCK];

    bool indirect_ptrs_fetched = 0;
    while (cur_dblock < num_dblocks)
    {
        uint32_t dblock = -1;

        // direct pointers
        if (cur_dblock < 5)
        {
            dblock = in.direct[cur_dblock];
        }
        // in-direct pointer
        else if (cur_dblock < (5 + PTRS_PER_BLOCK))
        {
            if (!indirect_ptrs_fetched)
            {
                indirect_ptrs_fetched = 1;
                read_block(fs.diskptr, in.indirect + fs.sb.data_block_idx, indirect_ptrs);

                // remove the indirect block
                flip_bit_in_bmap(fs.data_bmap, in.indirect, fs.sb.data_block_bitmap_idx);
            }
            dblock = indirect_ptrs[cur_dblock - 5];
        }
        else
        {
            // out of range
            return ERR;
        }

        flip_bit_in_bmap(fs.data_bmap, dblock, fs.sb.data_block_bitmap_idx);
        cur_dblock++;
    }

    return SUCC;
}

int stat(int inumber)
{
    if (check_valid_inode(inumber) == INVALID)
        return ERR;

    int inode_block_idx, inode_num;
    if (get_inode_from_disk(inumber, &inode_block_idx, &inode_num) == ERR)
    {
        return ERR;
    }

    inode in = ((inode *)fs.rblock)[inode_num];
    if (in.valid == INVALID)
    {
        return ERR;
    }

    int num_dblocks = ceil(in.size / BLOCKSIZE);
    int direct_ptrs = (num_dblocks > 5) ? 5 : num_dblocks;
    int indirect_ptrs = (num_dblocks > 5) ? num_dblocks - 5 : 0;

    printf("\ninode %d stats:\n", inumber);
    printf("Logical size: %d\n", in.size);
    printf("Number of data blocks in use: %d\n", num_dblocks);
    printf("Number of direct pointers used: %d\n", direct_ptrs);
    printf("Number of indirect pointers used: %d\n", indirect_ptrs);

    return SUCC;
}

bool check_valid_offset(int offset, int size)
{
    if (offset < 0 || offset >= size)
        return INVALID;

    return VALID;
}

uint32_t min(uint32_t a, uint32_t b)
{
    if (a < b)
        return a;
    return b;
}

int read_i(int inumber, char *data, int length, int offset)
{
    if (check_valid_inode(inumber) == INVALID)
        return ERR;

    int inode_block_idx, inode_num;
    if (get_inode_from_disk(inumber, &inode_block_idx, &inode_num) == ERR)
    {
        return ERR;
    }

    inode in = ((inode *)fs.rblock)[inode_num];
    if (in.valid == INVALID)
    {
        return ERR;
    }

    if (check_valid_offset(offset, in.size) == INVALID)
    {
        return ERR;
    }

    length += offset;
    if (length > in.size)
        length = in.size;

    uint32_t cur_dblock = offset / BLOCKSIZE, num_dblocks = ceil(length / BLOCKSIZE);
    uint32_t indirect_ptrs[PTRS_PER_BLOCK];
    bool indirect_ptrs_fetched = 0, start = 1;

    uint32_t data_offset = -1;
    while (cur_dblock < num_dblocks)
    {
        uint32_t dblock = -1;

        // direct pointers
        if (cur_dblock < 5)
        {
            dblock = in.direct[cur_dblock];
        }
        // in-direct pointer
        else if (cur_dblock < (5 + PTRS_PER_BLOCK))
        {
            if (!indirect_ptrs_fetched)
            {
                indirect_ptrs_fetched = 1;
                read_block(fs.diskptr, in.indirect + fs.sb.data_block_idx, indirect_ptrs);
            }
            dblock = indirect_ptrs[cur_dblock - 5];
        }
        else
        {
            // out of range
            return ERR;
        }

        char data_block[BLOCKSIZE];
        read_block(fs.diskptr, dblock + fs.sb.data_block_idx, data_block);

        if (data_offset == -1)
        {
            data_offset = min((cur_dblock + 1) * BLOCKSIZE, length) - offset;
            strncpy(data, data_block + (offset - cur_dblock * BLOCKSIZE), data_offset);
        }
        else
        {
            uint32_t copy_len = min(length - cur_dblock * BLOCKSIZE, BLOCKSIZE);
            strncpy(data + data_offset, data_block, copy_len);
            data_offset += copy_len;
        }

        cur_dblock++;
    }

    return data_offset;
}

int write_i(int inumber, char *data, int length, int offset)
{
    if (check_valid_inode(inumber) == INVALID)
        return ERR;

    int inode_block_idx, inode_num;
    if (get_inode_from_disk(inumber, &inode_block_idx, &inode_num) == ERR)
    {
        return ERR;
    }

    inode in = ((inode *)fs.rblock)[inode_num];
    if (in.valid == INVALID)
    {
        return ERR;
    }

    if (check_valid_offset(offset, in.size) == INVALID)
    {
        return ERR;
    }

    length += offset;
    if (length > in.size)
        length = in.size;

    uint32_t cur_dblock = offset / BLOCKSIZE, num_dblocks = ceil(length / BLOCKSIZE);
    uint32_t indirect_ptrs[PTRS_PER_BLOCK];
    bool indirect_ptrs_fetched = 0, start = 1;

    uint32_t data_offset = -1;
    while (cur_dblock < num_dblocks)
    {
        uint32_t dblock = -1;

        // direct pointers
        if (cur_dblock < 5)
        {
            dblock = in.direct[cur_dblock];
        }
        // in-direct pointer
        else if (cur_dblock < (5 + PTRS_PER_BLOCK))
        {
            if (!indirect_ptrs_fetched)
            {
                indirect_ptrs_fetched = 1;
                read_block(fs.diskptr, in.indirect + fs.sb.data_block_idx, indirect_ptrs);
            }
            dblock = indirect_ptrs[cur_dblock - 5];
        }
        else
        {
            // out of range
            return ERR;
        }

        char data_block[BLOCKSIZE];
        read_block(fs.diskptr, dblock + fs.sb.data_block_idx, data_block);

        if (data_offset == -1)
        {
            data_offset = min((cur_dblock + 1) * BLOCKSIZE, length) - offset;
            strncpy(data, data_block + (offset - cur_dblock * BLOCKSIZE), data_offset);
        }
        else
        {
            uint32_t copy_len = min(length - cur_dblock * BLOCKSIZE, BLOCKSIZE);
            strncpy(data + data_offset, data_block, copy_len);
            data_offset += copy_len;
        }

        cur_dblock++;
    }

    return SUCC;
}
