#include "sfs.h"

static mounted_fs fs = {.diskptr = NULL};

static int ceil_int(int a, int b)
{
    return (a + b - 1) / b;
}

static int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}

static int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

bool check_valid_inode(int inumber)
{
    if (inumber < 0 || inumber > fs.sb.inodes)
        return INVALID;

    return VALID;
}

void serialize(dir_entry *dir_data, char *data)
{
    int *q = (int *)data;
    *q = dir_data->valid;
    q++;
    *q = dir_data->type;
    q++;

    char *p = (char *)q;
    int i = 0;
    while (i < MAX_FILENAME_LEN)
    {
        *p = dir_data->name[i];
        p++;
        i++;
    }
    uint32_t *r = (uint32_t *)p;
    *r = dir_data->name_len;
    r++;
    *r = dir_data->inumber;
    r++;
}

void deserialize(dir_entry *dir_data, char *data)
{
    int *q = (int *)data;
    dir_data->valid = *q;
    q++;
    dir_data->type = *q;
    q++;

    char *p = (char *)q;
    int i = 0;
    while (i < MAX_FILENAME_LEN)
    {
        dir_data->name[i] = *p;
        p++;
        i++;
    }
    uint32_t *r = (uint32_t *)p;
    dir_data->name_len = *r;
    r++;
    dir_data->inumber = *r;
    r++;
}

void flip_bit_in_bmap(uint32_t *bmap, uint32_t bit_position, uint32_t offset_block_idx)
{
    int bmap_block_idx = bit_position / (8 * BLOCKSIZE);
    int bit_offset = bit_position % (BLOCKSIZE * 8);

    int bmap_block[BLOCKSIZE];
    read_block(fs.diskptr, offset_block_idx + bmap_block_idx, bmap_block);

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
                // free bit
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
    flip_bit_in_bmap(bmap, idx, bmap_offset);

    return idx;
}

int get_inode_from_disk(int inode_idx, int *inode_block_idx, int *inode_num)
{
    *inode_block_idx = inode_idx / INODE_PER_BLOCK;
    *inode_num = inode_idx % INODE_PER_BLOCK;

    if (read_block(fs.diskptr, fs.sb.inode_block_idx + (*inode_block_idx), fs.rblock) == ERR)
    {
        return ERR;
    }

    return SUCC;
}

int format(disk *diskptr)
{
    if (diskptr == NULL)
        return ERR;

    uint32_t N = diskptr->blocks;
    uint32_t M = N - 1;
    uint32_t I = 0.1 * M;

    uint32_t num_inodes = I * INODE_PER_BLOCK;
    uint32_t len_inode_bitmaps = num_inodes;
    uint32_t IB = ceil_int(len_inode_bitmaps, BITS_PER_BYTES * BLOCKSIZE);

    // remaining blocks for data blocks and data bit maps
    uint32_t R = M - I - IB;
    uint32_t len_data_bitmaps = R;
    uint32_t DBB = ceil_int(len_data_bitmaps, BITS_PER_BYTES * BLOCKSIZE);
    uint32_t DB = R - DBB;
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
            free(ptr_zero);
            return ERR;
        }
    }
    free(ptr_zero);

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
            free(inodes_array);
            return ERR;
        }
    }
    free(inodes_array);

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
    fs.root_dir = (dir_entry *)malloc(sizeof(dir_entry));
    *(fs.root_dir) = (dir_entry){.valid = VALID, .type = DIR, .name = "/", .name_len = 1, .inumber = 0};
    get_free_bmap_idx(fs.inode_bmap, fs.len_inode_bmap, fs.sb.inode_bitmap_block_idx);

    inode temp[INODE_PER_BLOCK];

    read_block(fs.diskptr, fs.sb.inode_block_idx, temp);
    temp[0].size = 0;
    temp[0].valid = VALID;

    write_block(fs.diskptr, fs.sb.inode_block_idx, temp);
}

int mount(disk *diskptr)
{

    // already mounted
    if (diskptr == NULL || fs.diskptr != NULL)
    {
        return ERR;
    }

    super_block sb;
    char buf[BLOCKSIZE];
    if (read_block(diskptr, 0, buf) == ERR)
    {
        return ERR;
    }

    memcpy(&sb, buf, sizeof(sb));
    if (sb.magic_number != MAGIC)
    {
        return ERR;
    }

    init_mounted_fs(&sb, diskptr);
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
    new_ptr = (inode *)fs.rblock; // typecasting in inode* type;
    new_ptr += inode_num;         // used for incrementing the blockptr to get to specific inode
    new_ptr->size = 0;
    new_ptr->valid = VALID;

    if (write_block(fs.diskptr, fs.sb.inode_block_idx + inode_block_idx, fs.rblock) == ERR) // writing back updates block into disk
        return ERR;

    return inode_idx;
}

int remove_file(int inumber)
{
    if (check_valid_inode(inumber) == INVALID)
        return ERR;

    // set inode.valid = 0
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
    if (write_block(fs.diskptr, fs.sb.inode_block_idx + inode_block_idx, fs.rblock) == ERR)
    {
        return ERR;
    }

    // update inode bmaps
    flip_bit_in_bmap(fs.inode_bmap, inumber, fs.sb.inode_bitmap_block_idx);

    // update data bmaps
    uint32_t cur_dblock = 0, num_dblocks = ceil_int(in.size, BLOCKSIZE);
    uint32_t indirect_ptrs[PTRS_PER_BLOCK];

    bool indirect_ptrs_fetched = 0;

    while (cur_dblock < num_dblocks)
    {
        int dblock = -1;

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
                if (read_block(fs.diskptr, in.indirect + fs.sb.data_block_idx, indirect_ptrs) == ERR)
                {
                    return ERR;
                }

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

    int num_dblocks = ceil_int(in.size, BLOCKSIZE);
    int direct_ptrs = min(num_dblocks, 5);
    int indirect_ptrs = max(num_dblocks - 5, 0);

    printf("\n[inode %d stats]:\n", inumber);
    printf("Logical size: %d\n", in.size);
    printf("Valid: %d\n", in.valid);
    printf("Number of data blocks in use: %d\n", num_dblocks);
    printf("Number of direct pointers used: %d\n", direct_ptrs);
    printf("Number of indirect pointers used: %d\n", indirect_ptrs);

    return SUCC;
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

    if (offset < 0 || offset >= in.size)
    {
        return ERR;
    }

    length = min(in.size, length + offset);

    uint32_t cur_dblock = offset / BLOCKSIZE, num_dblocks = ceil_int(length, BLOCKSIZE);
    uint32_t indirect_ptrs[PTRS_PER_BLOCK];
    bool indirect_ptrs_fetched = 0;

    int data_offset = -1;
    while (cur_dblock < num_dblocks)
    {
        int dblock = -1;

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
                if (read_block(fs.diskptr, in.indirect + fs.sb.data_block_idx, indirect_ptrs) == ERR)
                {
                    return ERR;
                }
            }
            dblock = indirect_ptrs[cur_dblock - 5];
        }
        else
        {
            // out of range, dead zone x_x
            return ERR;
        }

        char data_block[BLOCKSIZE];
        if (read_block(fs.diskptr, dblock + fs.sb.data_block_idx, data_block) == ERR)
        {
            return ERR;
        }

        if (data_offset == -1)
        {
            data_offset = min((cur_dblock + 1) * BLOCKSIZE, length) - offset;
            memcpy(data, data_block + (offset - cur_dblock * BLOCKSIZE), data_offset);
        }
        else
        {
            int copy_len = min(length - cur_dblock * BLOCKSIZE, BLOCKSIZE);
            memcpy(data + data_offset, data_block, copy_len);
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

    if (offset < 0 || offset > in.size)
    {
        return ERR;
    }

    length += offset;

    uint32_t cur_dblock = offset / BLOCKSIZE, num_dblocks = ceil_int(length, BLOCKSIZE);
    uint32_t indirect_ptrs[PTRS_PER_BLOCK];
    bool indirect_ptrs_fetched = 0;

    int data_offset = -1;

    bool indirect_ptrs_def = 0;
    uint32_t file_dblocks = ceil_int(in.size, BLOCKSIZE); // number of dblocks pointers already in inode

    while (cur_dblock < num_dblocks)
    {
        int dblock = -1;
        // overwrite
        if (cur_dblock < file_dblocks)
        {
            if (cur_dblock < 5)
            {
                dblock = in.direct[cur_dblock];
            }
            else if (cur_dblock < (5 + PTRS_PER_BLOCK))
            {
                if (!indirect_ptrs_fetched)
                {
                    indirect_ptrs_fetched = 1;
                    if (read_block(fs.diskptr, in.indirect + fs.sb.data_block_idx, indirect_ptrs) == ERR)
                    {
                        return ERR;
                    }
                }
                dblock = indirect_ptrs[cur_dblock - 5];
            }
        }
        // append
        else if (cur_dblock >= file_dblocks)
        {
            // no data block empty, disk full
            if ((dblock = get_free_bmap_idx(fs.data_bmap, fs.len_data_bmap, fs.sb.data_block_bitmap_idx)) == ERR)
            {
                break;
            }
            else if (cur_dblock < 5)
            {
                in.direct[cur_dblock] = dblock;
            }
            else if (cur_dblock < (5 + PTRS_PER_BLOCK))
            {
                if (cur_dblock == 5)
                {
                    // allocate indirect pointers data block
                    if ((in.indirect = get_free_bmap_idx(fs.data_bmap, fs.len_data_bmap, fs.sb.data_block_bitmap_idx)) == ERR)
                    {
                        break;
                    }
                }
                indirect_ptrs_def = 1;
                indirect_ptrs[cur_dblock - 5] = dblock;
            }
            // exceeds maximum file size
            else
            {
                break;
            }
        }

        // read the data block(needed when appending)
        char data_block[BLOCKSIZE];
        if (read_block(fs.diskptr, dblock + fs.sb.data_block_idx, data_block) == ERR)
        {
            return ERR;
        }

        // first block(writing suffix)
        if (data_offset == -1)
        {
            data_offset = min((cur_dblock + 1) * BLOCKSIZE, length) - offset;
            memcpy(data_block + (offset - cur_dblock * BLOCKSIZE), data, data_offset);
        }
        // other blocks(writing prefix)
        else
        {
            uint32_t copy_len = min(length - cur_dblock * BLOCKSIZE, BLOCKSIZE);
            memcpy(data_block, data + data_offset, copy_len);
            data_offset += copy_len;
        }

        // write the data block
        if (write_block(fs.diskptr, dblock + fs.sb.data_block_idx, data_block) == ERR)
        {
            return ERR;
        }
        cur_dblock++;
    }

    // update file size
    in.size = in.size + max(length - in.size, 0);

    // if indirect pointers created
    if (indirect_ptrs_def && write_block(fs.diskptr, in.indirect + fs.sb.data_block_idx, indirect_ptrs) == ERR)
    {
        return ERR;
    }

    // update inode block
    ((inode *)(fs.rblock))[inode_num] = in;
    if (write_block(fs.diskptr, fs.sb.inode_block_idx + inode_block_idx, fs.rblock) == ERR)
    {
        return ERR;
    }

    return data_offset;
}

char **parse_path(char *path, int *size)
{
    char **names = (char **)malloc(MAX_DEPTH_ALLOWED * sizeof(char *));
    char *token;
    char *path1=(char *)malloc(strlen(path)*sizeof(char));

    strcpy(path1, path);

    token = strtok(path1, "/");
    while (token != NULL)
    {
        names[*size] = (char *)malloc(strlen(token) * sizeof(char));
        strcpy(names[(*size)++], token);
        token = strtok(NULL, "/");
    }

    return names;
}

dir_entry *get_dirblock(uint32_t dir_inode, uint32_t *num_dir_entry)
{
    int inode_block_idx, inode_num;
    get_inode_from_disk(dir_inode, &inode_block_idx, &inode_num);
    // printf("Dir entry size: %d, %d, %d\n", dir_inode, inode_block_idx, inode_num);

    inode in = ((inode *)fs.rblock)[inode_num];

    if (in.valid == INVALID)
    {
        return NULL;
    }

    *num_dir_entry = in.size / sizeof(dir_entry);

    // printf("entries: %d\n", *num_dir_entry);
    dir_entry *dir_entries = (dir_entry *)malloc((*num_dir_entry) * sizeof(dir_entry));

    char tmp[in.size];
    if (read_i(dir_inode, tmp, in.size, 0) == ERR)
    {
        free(dir_entries);
        return NULL;
    }
    for (int i = 0; i < *num_dir_entry; i++)
    {
        deserialize(dir_entries + i, tmp + i * sizeof(dir_entry));
    }

    for (int i = 0; i < *num_dir_entry; i++){
        printf("name: %s\n", dir_entries[i].name);
    }
    printf("\n");

    return dir_entries;
}

// return inumber of the next_dir
uint32_t walk_utils(uint32_t cur_dir_inode, char *next_dir)
{
    uint32_t num_dir_entry;
    dir_entry *dir_entries = get_dirblock(cur_dir_inode, &num_dir_entry);

    int i;
    for (i = 0; i < num_dir_entry; i++)
    {
        if (strcmp(dir_entries[i].name, next_dir) == 0 && dir_entries[i].valid == VALID && dir_entries[i].type == DIR)
            break;
    }
    if (i == num_dir_entry)
    {
        // dir not found
        free(dir_entries);
        return ERR;
    }

    uint32_t inum = dir_entries[i].inumber;
    free(dir_entries);
    return inum;
}

uint32_t get_parent_inode(char *path, char **base_name)
{
    int len_dir_names = 0;
    char **dir_names = parse_path(path, &len_dir_names);
    printf("path name %s\n", path);

    if (len_dir_names == 0)
    {
        return ERR;
    }
    *base_name = (char *)malloc(strlen(dir_names[len_dir_names - 1]) * sizeof(char));
    // base_name = dir_names[len_dir_names - 1];

    strcpy(*base_name, dir_names[len_dir_names - 1]);

    // printf("len dir name %d\n", len_dir_names);
    int cur_dir_inode = 0; // root_inode
    for (int i = 0; i < len_dir_names - 1; i++)
    {
        if ((cur_dir_inode = walk_utils(cur_dir_inode, dir_names[i])) == ERR)
        {
            return ERR;
        }
        printf("current inode: %d\n", cur_dir_inode);
    }

    // printf("In get_parent_inode %d\n", cur_dir_inode);
    for (int i = 0; i < len_dir_names; i++)
        free(dir_names[i]);

    return cur_dir_inode;
}

int create_dir(char *dir_path)
{
    char *base_name;
    uint32_t parent_inode;
    if ((parent_inode = get_parent_inode(dir_path, &base_name)) == ERR)
    {
        return ERR;
    }

    dir_entry new_dir;
    new_dir.valid = VALID;
    new_dir.type = DIR;

    strcpy(new_dir.name, base_name);
    new_dir.name_len = strlen(base_name);

    // dir already exists!
    if (walk_utils(parent_inode, base_name) != ERR)
    {
        return ERR;
    }

    if ((new_dir.inumber = create_file()) == ERR)
    {
        return ERR;
    }

    // parent file size
    uint32_t num_dir_entry;
    dir_entry *dir_entries = get_dirblock(parent_inode, &num_dir_entry);

    int i;
    for (i = 0; i < num_dir_entry; i++)
    {
        if (dir_entries[i].valid == INVALID)
        {
            break;
        }
    }
    free(dir_entries);

    // write new dir entry at parent DB
    uint32_t write_offset = i * (sizeof(dir_entry));

    char tmp[sizeof(dir_entry)];
    serialize(&new_dir, tmp);
    if (write_i(parent_inode, tmp, sizeof(dir_entry), write_offset) == ERR)
    {
        return ERR;
    }

    return SUCC;
}

int remove_dir_utils(uint32_t parent_inode)
{
    printf("parent inode in remove utils: %d\n", parent_inode);

    uint32_t num_dir_entry;
    dir_entry *dir_entries = get_dirblock(parent_inode, &num_dir_entry);

    for (int i = 0; i < num_dir_entry; i++)
    {
        if (dir_entries[i].valid == INVALID){
            continue;
        }
        else if(dir_entries[i].type == DIR){
            remove_dir_utils(dir_entries[i].inumber);
        }
        remove_file(dir_entries[i].inumber);
        dir_entries[i].valid = INVALID;
    }

    free(dir_entries);
    return SUCC;
}

int remove_dir(char *dir_path)
{
    char *base_name;
    uint32_t parent_inode = get_parent_inode(dir_path, &base_name);
    
    printf("parent inode in remove: %d\n", parent_inode);
    // get parent directory list
    // parent file size
    uint32_t num_dir_entry;
    dir_entry *dir_entries = get_dirblock(parent_inode, &num_dir_entry);

    int i;
    for (i = 0; i < num_dir_entry; i++)
    {
        if (strcmp(dir_entries[i].name, base_name) == 0 && dir_entries[i].type == DIR && dir_entries[i].valid == VALID)
        {
            remove_dir_utils(dir_entries[i].inumber);
            remove_file(dir_entries[i].inumber);

            dir_entries[i].valid = INVALID;
            break;
        }
    }
    if (i == num_dir_entry)
    {
        free(dir_entries);
        return ERR;
    }

    char tmp[sizeof(dir_entry)];
    serialize(dir_entries + i, tmp);

    uint32_t write_offset = i * sizeof(dir_entry);
    if (write_i(parent_inode, tmp, sizeof(dir_entry), write_offset) == ERR){
        free(dir_entries);
        return ERR;
    }

    free(dir_entries);
    return SUCC;
}

int read_file(char *filepath, char *data, int length, int offset)
{
    char *base_name;
    uint32_t parent_inode = 0;
    if((parent_inode = get_parent_inode(filepath, &base_name)) == ERR){
        // file path not found
        return ERR;
    }

    printf("Parent inode:%d\n",parent_inode);

    uint32_t num_dir_entry;
    dir_entry *dir_entries = get_dirblock(parent_inode, &num_dir_entry);

    int inumber = -1;
    for (int i = 0; i < num_dir_entry; i++)
    {
        if (strcmp(dir_entries[i].name, base_name) == 0 && dir_entries[i].type == FILE)
        {
            inumber = dir_entries[i].inumber;
            break;
        }
    }
    free(dir_entries);

    if (inumber == -1 || check_valid_inode(inumber) == INVALID)
    {
        return ERR;
    }

    return read_i(inumber, data, length, offset);
}

int write_file(char *filepath, char *data, int length, int offset)
{
    char *base_name;
    uint32_t parent_inode = get_parent_inode(filepath, &base_name);
    printf("In write_file %s\n", filepath);

    uint32_t num_dir_entry;
    dir_entry *dir_entries = get_dirblock(parent_inode, &num_dir_entry);

    int inumber = -1;
    for (int i = 0; i < num_dir_entry; i++)
    {
        if (strcmp(dir_entries[i].name, base_name) == 0 && dir_entries[i].type == FILE)
        {
            inumber = dir_entries[i].inumber;
            break;
        }
    }

    // create new file
    if (inumber == -1)
    {
        if ((inumber = create_file()) == ERR)
        {
            free(dir_entries);
            return ERR;
        }

        // update parent dir
        int i;
        for (i = 0; i < num_dir_entry; i++)
        {
            if (dir_entries[i].valid == INVALID)
            {
                break;
            }
        }
        dir_entry new_file = (dir_entry){.valid = VALID, .type = FILE, .inumber = inumber};
        strcpy(new_file.name, base_name);
        new_file.name_len = strlen(base_name);

        // write new file at parent DB
        uint32_t write_offset = i * (sizeof(dir_entry));

        char tmp[sizeof(dir_entry)];
        serialize(&new_file, tmp);
        if (write_i(parent_inode, tmp, sizeof(dir_entry), write_offset) == ERR)
        {
            free(dir_entries);
            return ERR;
        }
    }
    else if (check_valid_inode(inumber) == INVALID)
    {
        free(dir_entries);
        return ERR;
    }

    free(dir_entries);
    return write_i(inumber, data, length, offset);
}
