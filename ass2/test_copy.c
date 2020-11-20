#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>
#include<assert.h>

#define MAX_FILENAME_LEN 22

typedef struct dir_entry {
	int valid;
	unsigned int type;
	char name[22];
	uint32_t name_len;
	uint32_t inumber;
} dir_entry;

void serialize(dir_entry* dir_data, char *data)
{
    int *q = (int*)data;    
    *q = dir_data->valid;       q++;    
    *q = dir_data->type;        q++;    
    
    char *p = (char*)q;
    int i = 0;
    while (i < MAX_FILENAME_LEN)
    {
        *p = dir_data->name[i];
        p++;
        i++;
    }
    uint32_t *r = (uint32_t *)p;
    *r = dir_data->name_len;    r++;
    *r = dir_data->inumber;     r++;
}

void deserialize(dir_entry* dir_data, char *data){
    int *q = (int *)data;
    dir_data->valid = *q;       q++;
    dir_data->type = *q;        q++;

    char *p = (char *)q;
    int i = 0;
    while (i < MAX_FILENAME_LEN)
    {
        dir_data->name[i] = *p;
        p++;
        i++;
    }
    uint32_t *r = (uint32_t *)p;
    dir_data->name_len = *r;    r++;
    dir_data->inumber = *r;     r++;
}

int main(){
	dir_entry new_dir;
	new_dir.valid = 1;
    new_dir.type = 0;

    strcpy(new_dir.name, "base_name");    
    new_dir.name_len = strlen("base_name");
    new_dir.inumber = 35;
    char tmp[sizeof(dir_entry)];
    serialize(&new_dir, tmp);

    dir_entry des_dir;
    deserialize(&des_dir, tmp);
    
    printf("\nTemp: %s %d\n", des_dir.name, sizeof(tmp));
    assert(des_dir.inumber == new_dir.inumber);
    return 0;
}
