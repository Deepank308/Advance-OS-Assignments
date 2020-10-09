/*
Deepank Agrawal 17CS30011
Praagy Rastogi 17CS30026
Kirti Agarwal 20CS60R14

LKM built in kernel version 5.4.0
*/

#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <heap.h>


MODULE_LICENSE("GPL");
DEFINE_MUTEX(read_write_mutex); 
DEFINE_MUTEX(open_close_mutex);

#define MAX_PROCESS_LIMIT 128
#define MAX_BUFF_SIZE 256
#define MAX_HEAP 0xF0
#define MIN_HEAP 0xFF


typedef int32_t err_t;
static struct file_operations file_ops;
static char *buffer = NULL;
static int32_t buffer_len = 0;


// describe the state the process has PASSED
enum process_states {
    PROCESS_FILE_OPEND,
    PROCESS_HEAP_INITD,
    PROCESS_HEAP_WRITE,
    PROCESS_FILE_CLOSE,
};

// single process
typedef struct process_state_struct{
    pid_t pid;
    enum process_states state;
    struct heap *lkm_heap;

} process_state_struct;


// list of processes
typedef struct{
    struct process_state_struct* state_list;
    size_t count;
} processes_list;

static processes_list *processes;


// get the idx for pid or the first idx which is empty
size_t get_idx_from_pid(pid_t pid){

    size_t process_idx = -1, i = 0;
    
    for(i = 0; i < MAX_PROCESS_LIMIT; i++){
        if(process_idx == -1 && processes->state_list[i].state == PROCESS_FILE_CLOSE){
            process_idx = i;
        }
        if(processes->state_list[i].pid == pid){
            process_idx = i;
            break;
        }
    }
    
    return process_idx;
}


// store heap type and size
static err_t init_heap_attr(size_t process_idx){

    int32_t heap_type;
    size_t heap_size;
    unsigned char hex_type;
    
    if(buffer_len != 2){
        return -EINVAL;
    }
    
    hex_type = buffer[0];
    switch(hex_type){
        case MIN_HEAP:
            heap_type = 0;
            break;

        case MAX_HEAP:
            heap_type = 1;
            break;

        default:
            return -EINVAL;
    }

    heap_size = (unsigned char)buffer[1];
    
    if(heap_size < 1 || heap_size > 100){
        return -EINVAL;
    }
    return create_heap(&processes->state_list[process_idx].lkm_heap, heap_type, heap_size);
}


// insert data into heap
static ssize_t insert_heap_data(size_t process_idx){

    int32_t val = 0;
    
    if(buffer_len != sizeof(int32_t)){
        return -EINVAL;
    }

    val = *((int32_t *) buffer);

    if(insert(processes->state_list[process_idx].lkm_heap, val) == -1) return -EACCES;
    else return buffer_len;
}


// handle write on the lkm kernel for the process depending on the state of the process
static ssize_t handle_process_write(size_t process_idx){

    int32_t process_state = processes->state_list[process_idx].state;
    err_t err = 0;

    // read the size and type of heap
    if(process_state == PROCESS_FILE_OPEND){
        if((err = init_heap_attr(process_idx)) >= 0){
            processes->state_list[process_idx].state = PROCESS_HEAP_INITD;
        }
    }
    // read data into the heap
    else if(process_state >= PROCESS_HEAP_INITD){
        if((err = insert_heap_data(process_idx)) >= 0){
            processes->state_list[process_idx].state = PROCESS_HEAP_WRITE;
        }
    }
    else err = -EACCES;

    return err;
}


// `write` from user's pov
static ssize_t file_write(struct file *file, const char *buf, size_t count, loff_t *pos){
    
    size_t process_idx = get_idx_from_pid(current->pid);
    err_t err = 0;

    // printk(KERN_ALERT "PID: %d", (int)current->pid);

    // process not found
    if(process_idx < 0 || processes->state_list[process_idx].pid != current->pid){
        return -ESRCH;
    }

    if(!buf || !count) return -EINVAL;
    
    mutex_lock(&read_write_mutex);
    
    buffer_len = count < 256 ? count:256;
    if(copy_from_user(buffer, buf, buffer_len)) err = -ENOBUFS;
    
    if(!err) err = handle_process_write(process_idx);
    
    if(!err) err = buffer_len;
    
    mutex_unlock(&read_write_mutex);
    
    return err;
}


static ssize_t handle_process_read(size_t process_idx){

    int32_t process_state = processes->state_list[process_idx].state;
    int32_t top;
    
    if(process_state < PROCESS_HEAP_INITD){
        return -EACCES;
    }
    
    if(extract(processes->state_list[process_idx].lkm_heap, &top) == -1) return -EACCES;
    
    // printk(KERN_ALERT "Top: %d", top);
    
    // put value to buffer
    buffer[0] = (top >> 0)  & 0xFF;
    buffer[1] = (top >> 8)  & 0xFF;
    buffer[2] = (top >> 16) & 0xFF;
    buffer[3] = (top >> 24) & 0xFF;
    
    return 0;
}


// `read` from user's pov
static ssize_t file_read(struct file *file, char *buf, size_t count, loff_t *pos){

    size_t process_idx = get_idx_from_pid(current->pid);
    err_t err = 0;

    // printk(KERN_ALERT "PID: %d", (int)current->pid);
    
    // process not found
    if(process_idx < 0 || processes->state_list[process_idx].pid != current->pid){
        return -ESRCH;
    }

    if(!buf || !count) return -EINVAL;

    mutex_lock(&read_write_mutex);
    
    buffer_len = count < 256 ? count:256;
    err = handle_process_read(process_idx);
    
    if(!err && copy_to_user(buf, buffer, buffer_len)) err = -ENOBUFS;

    if(!err) err = buffer_len;
    mutex_unlock(&read_write_mutex);

    return err;
}


static err_t file_open(struct inode *inode, struct file *file){
    
    err_t err = 0;
    
    mutex_lock(&open_close_mutex);
    size_t process_idx = get_idx_from_pid(current->pid);
    
    // cannot be accomodated or already open
    if(process_idx < 0 || processes->state_list[process_idx].state != PROCESS_FILE_CLOSE){
        err = -EMFILE;
    }
    else
    {
        printk(KERN_ALERT "------------File open %d\n---------------", (int)current->pid);
        processes->state_list[process_idx].state = PROCESS_FILE_OPEND;
        processes->state_list[process_idx].pid = current->pid;
        processes->count++;
    }
    mutex_unlock(&open_close_mutex);

    return err;
}


static err_t file_close(struct inode *inode, struct file *file){

    err_t err = 0;

    mutex_lock(&open_close_mutex);

    size_t process_idx = get_idx_from_pid(current->pid);
    
    // process not found
    if(process_idx < 0 || processes->state_list[process_idx].pid != current->pid){
        err = -ESRCH;
    }
    else
    {
        printk(KERN_ALERT "------------File close %d\n---------------", (int)current->pid);
        
        if(processes->state_list[process_idx].state >= PROCESS_HEAP_INITD)
            destroy_heap(processes->state_list[process_idx].lkm_heap);

        processes->state_list[process_idx].state = PROCESS_FILE_CLOSE;
        processes->state_list[process_idx].pid = -1;
        processes->count--;
    }

    mutex_unlock(&open_close_mutex);	

    return err;
}


// initializes the process list and their heaps
static void init_process_state_list(void){
    
    size_t i = 0;

    processes = (processes_list *)vmalloc(sizeof(processes_list));
    processes->count = 0;

    processes->state_list = (process_state_struct *)vmalloc(MAX_PROCESS_LIMIT*sizeof(process_state_struct));

    for(i = 0; i < MAX_PROCESS_LIMIT; i++){
        processes->state_list[i].pid = -1;
        processes->state_list[i].state = PROCESS_FILE_CLOSE;
        processes->state_list[i].lkm_heap = NULL;
    }

    return;
}


static err_t lkm_init(void){
    // create proc file
    struct proc_dir_entry *proc_file = proc_create("partb_1_17CS30011_17CS30026_20CS60R14", 0666, NULL, &file_ops);
    if(!proc_file) return -ENOENT;
    
    // register pointers
    file_ops.owner = THIS_MODULE;
    file_ops.open = file_open;
    file_ops.write = file_write;
    file_ops.read = file_read;
    file_ops.release = file_close;
    
    buffer = (char *)vmalloc(MAX_BUFF_SIZE*sizeof(char));

    // process state array
    init_process_state_list();

    printk(KERN_ALERT "Initialized PB1-LKM\n");
    return 0;
}


static void lkm_exit(void){
    remove_proc_entry("partb_1_17CS30011_17CS30026_20CS60R14", NULL);

    vfree(processes);

    printk(KERN_ALERT "Removed PB1-LKM\n");
}


module_init(lkm_init);
module_exit(lkm_exit);

