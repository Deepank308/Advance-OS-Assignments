#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

#include "disk.h"

#define DIR 0
#define FILE 1
#define BITS_PER_BYTES 8
#define PTRS_PER_BLOCK BLOCKSIZE / 4    // 1024
#define INODE_PER_BLOCK BLOCKSIZE / 32  // 128
#define MAX_FILE PTRS_PER_BLOCK * 128
#define MAX_FILENAME_LEN 22
#define MAX_DEPTH_ALLOWED 32

const static uint32_t MAGIC = 12345;

typedef struct inode {
	uint32_t valid; // 0 if invalid
	uint32_t size; // logical size of the file
	uint32_t direct[5]; // direct data block pointer
	uint32_t indirect; // indirect pointer
} inode;


typedef struct dir_entry {
	bool valid;
	bool type;
	char name[MAX_FILENAME_LEN];
	uint32_t name_len;
	uint32_t inumber;
} dir_entry;


typedef struct super_block {
	uint32_t magic_number;	// File system magic number
	uint32_t blocks;	// Number of blocks in file system (except super block)

	uint32_t inode_blocks;	// Number of blocks reserved for inodes == 10% of Blocks
	uint32_t inodes;	// Number of inodes in file system == length of inode bit map
	uint32_t inode_bitmap_block_idx;  // Block Number of the first inode bit map block
	uint32_t inode_block_idx;	// Block Number of the first inode block

	uint32_t data_block_bitmap_idx;	// Block number of the first data bitmap block
	uint32_t data_block_idx;	// Block number of the first data block
	uint32_t data_blocks;  // Number of blocks reserved as data blocks
} super_block;


typedef struct mounted_fs {
	super_block sb;
	
	uint32_t* inode_bmap;
	uint32_t* data_bmap;
	void* rblock;

	uint32_t len_inode_bmap;
	uint32_t len_data_bmap;

	disk* diskptr;
	dir_entry* root_dir;
} mounted_fs;


int format(disk *diskptr);

int mount(disk *diskptr);

int create_file();

int remove_file(int inumber);

int stat(int inumber);

int read_i(int inumber, char *data, int length, int offset);

int write_i(int inumber, char *data, int length, int offset);


int read_file(char *filepath, char *data, int length, int offset);
int write_file(char *filepath, char *data, int length, int offset);
int create_dir(char *dirpath);
int remove_dir(char *dirpath);
 