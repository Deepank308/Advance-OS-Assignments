#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>

typedef struct dir_entry {
	int valid;
	unsigned int type;
	char name[22];
	uint32_t name_len;
	uint32_t inumber;
} dir_entry;
// char* custom_memcpy(char *dest, char* src, uint32_t size){
// 	printf("\n %d",*(int *)src);
// 	printf("\n %s", src);
//     memcpy(dest, src, size);
//     return(dest+size);
// }
// void struct_to_char(char *dest, dir_entry dir_data){
//     dest=custom_memcpy(dest,&dir_data.valid, sizeof(dir_data.valid));
//     dest=custom_memcpy(dest,&dir_data.type, sizeof(dir_data.type));
//     dest=custom_memcpy(dest, dir_data.name, sizeof(dir_data.name));
//     dest=custom_memcpy(dest, &dir_data.name_len, sizeof(dir_data.name_len));
//     dest=custom_memcpy(dest, &dir_data.inumber, sizeof(dir_data.inumber));
// }

void struct_to_char(char *dest, dir_entry dir_data){	
	memcpy(dest,(char *)&dir_data.valid, sizeof(int));
	// printf("\n%d, %s",dir_data.valid,dest);
	memcpy(dest+=sizeof(dir_data.valid), (char *)&dir_data.type, sizeof(dir_data.type));
	memcpy(dest+=sizeof(dir_data.type), dir_data.name, sizeof(dir_data.name));
	memcpy(dest+=sizeof(dir_data.name), (char *)&dir_data.name_len, sizeof(dir_data.name_len));
	memcpy(dest+=sizeof(dir_data.name_len), (char *)&dir_data.inumber, sizeof(dir_data.inumber));
}
void serialize(dir_entry* dir_data, char *data)
{
    int *q = (int*)data;    
    *q = dir_data->valid;       q++;    
    *q = dir_data->type;        q++;    
    

    char *p = (char*)q;
    int i = 0;
    while (i < dir_data->name_len)
    {
        *p = dir_data->namem[i];
        p++;
        i++;
    }
    *q = dir_data->name_len;    q++;
    *q = dir_data->inumber;     q++;
}
int main(){
	dir_entry new_dir;
	new_dir.valid = 1;
    new_dir.type = 0;

    strcpy(new_dir.name, "base_name");    
    new_dir.name_len = strlen("base_name");
    new_dir.inumber=35;
    char tmp[sizeof(dir_entry)];
    struct_to_char( new_dir);
    printf("\nTemp: %s %d\n", tmp, sizeof(tmp));

}
