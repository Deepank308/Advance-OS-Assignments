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
#include <ioctl_def.h>

MODULE_LICENSE("GPL");
DEFINE_MUTEX(open_close_mutex);

#define MAX_PROCESS_LIMIT 128
#define MAX_HEAP 1
#define MIN_HEAP 0

typedef int32_t err_t;
static struct file_operations file_ops;

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
static size_t get_idx_from_pid(pid_t pid){

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


static err_t init_heap_attr(size_t process_idx, struct pb2_set_type_arguments *heap_init_param){

    if(heap_init_param == NULL)
        return -EINVAL;

    if(heap_init_param->heap_size > 0 && heap_init_param->heap_size <= 100 
        && (heap_init_param->heap_type == MIN_HEAP || heap_init_param->heap_type == MAX_HEAP)){

        printk(KERN_ALERT "Creating heap %d \n", (int)current->pid);
        return create_heap(&processes->state_list[process_idx].lkm_heap, heap_init_param->heap_type, heap_init_param->heap_size);
    }

    return -EINVAL;
}


static err_t handle_pb2_set_type(struct pb2_set_type_arguments *heap_init_param, size_t process_idx){
    
    err_t err = 0;
    if(processes->state_list[process_idx].state > PROCESS_FILE_OPEND &&
        processes->state_list[process_idx].state < PROCESS_FILE_CLOSE ){
        destroy_heap(processes->state_list[process_idx].lkm_heap);
    }

    err = init_heap_attr(process_idx, heap_init_param);

    if(!err)
        processes->state_list[process_idx].state = PROCESS_HEAP_INITD;

    return err;
}


static err_t handle_pb2_insert(int32_t *val, size_t process_idx){
    
    err_t err = 0;
    if(processes->state_list[process_idx].state < PROCESS_HEAP_INITD)
        return -EACCES;

    if(val == NULL)
        return -EINVAL;
    
    printk(KERN_ALERT "Insert heap %d by %d \n", *val, (int)current->pid);
    err = insert(processes->state_list[process_idx].lkm_heap, *val);

    if(!err)
        processes->state_list[process_idx].state = PROCESS_HEAP_WRITE;
    else
        return -EACCES;

    return err;
}


static err_t handle_pb2_extract(struct result *heap_extract, size_t process_idx){   
    
    if(processes->state_list[process_idx].state < PROCESS_HEAP_INITD)
        return -EACCES;

    if(heap_extract == NULL){
        heap_extract = (struct result *)vmalloc(sizeof(result));
    }

    heap_extract->heap_size = processes->state_list[process_idx].lkm_heap->count;  
    return extract(processes->state_list[process_idx].lkm_heap, &heap_extract->result);
}


static err_t handle_pb2_get_info(struct obj_info *heap_state_info, size_t process_idx){

    if(processes->state_list[process_idx].state < PROCESS_HEAP_INITD)
        return -EACCES;

    if(heap_state_info == NULL){
        heap_state_info = (struct obj_info *)vmalloc(sizeof(obj_info));
    }

    printk(KERN_ALERT "Get Info HEAP %d \n", (int)current->pid);

    struct heap *process_heap = processes->state_list[process_idx].lkm_heap;

    // put the heap data
    heap_state_info->heap_size = process_heap->count;
    heap_state_info->heap_type = process_heap->type;
    heap_state_info->last_inserted = process_heap->last_inserted;

    return get_root(process_heap, &heap_state_info->root);
}


// handle IOCTL commands to the file
static long file_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

    err_t err = 0;
    size_t process_idx = get_idx_from_pid(current->pid);

    // process not found
    if(process_idx < 0 || processes->state_list[process_idx].pid != current->pid)
        return -ESRCH;

    switch (cmd){
        case PB2_SET_TYPE:
        {
            // handle heap init
            struct pb2_set_type_arguments heap_init_param;
            if(!copy_from_user(&heap_init_param, (struct pb2_set_type_arguments *)arg, sizeof(heap_init_param))){

                printk(KERN_ALERT "Set: heap type %d \n", heap_init_param.heap_type);
                err = handle_pb2_set_type(&heap_init_param, process_idx);
            }
            else err = -ENOBUFS;
            return err;
        }
        case PB2_INSERT:
        {
            // handle heap insert
            int32_t val;
            if(!copy_from_user(&val, (int32_t *)arg, sizeof(val))){
                err = handle_pb2_insert(&val, process_idx);
            }
            else err = -ENOBUFS;
            return err;
        }
        case PB2_GET_INFO:
        {
            // handle heap current data
            struct obj_info heap_state_info;
            printk(KERN_ALERT "Get info: \n");
            err = handle_pb2_get_info(&heap_state_info, process_idx);
            if(!err && copy_to_user((struct obj_info *)arg, &heap_state_info, sizeof(heap_state_info))){
                err = -ENOBUFS;
            }
            return err;
        }
        case PB2_EXTRACT:
        {
            // handle extract-min/max from heap
            struct result heap_extract;
            err = handle_pb2_extract(&heap_extract, process_idx);
            if(!err && copy_to_user((struct result *)arg, &heap_extract, sizeof(heap_extract))){
                err = -ENOBUFS;
            }
            return err;
        }
        default:
            return -EINVAL;
    }

    return -EINVAL;
}


static err_t file_open(struct inode *inode, struct file *file){
    
    err_t err = 0;

    mutex_lock(&open_close_mutex);

    size_t process_idx = get_idx_from_pid(current->pid);

    // cannot be accomodated or already open
    if(process_idx < 0 || processes->state_list[process_idx].state != PROCESS_FILE_CLOSE)
        err = -EMFILE;
    else
    {
        printk(KERN_ALERT "File open %d\n", (int) current->pid);
        processes->state_list[process_idx].state = PROCESS_FILE_OPEND;
        processes->state_list[process_idx].pid = current->pid;
        processes->count++;
    }

    mutex_unlock(&open_close_mutex);

    printk(KERN_ALERT "------------File open %d\n---------------", (int)current->pid);

    return err;
}


static err_t file_close(struct inode *inode, struct file *file){

    err_t err = 0;

    mutex_lock(&open_close_mutex);

    size_t process_idx = get_idx_from_pid(current->pid);
    
    // process not found
    if(process_idx < 0 || processes->state_list[process_idx].pid != current->pid)
        err = -ESRCH;
    else
    {
        printk(KERN_ALERT "File close %d\n", (int)current->pid);
        
        if(processes->state_list[process_idx].state >= PROCESS_HEAP_INITD)
        	destroy_heap(processes->state_list[process_idx].lkm_heap);
        	
        processes->state_list[process_idx].state = PROCESS_FILE_CLOSE;
        processes->state_list[process_idx].pid = -1;
        processes->count--;
    }

    mutex_unlock(&open_close_mutex);

    printk(KERN_ALERT "------------File close %d\n---------------", (int)current->pid);
    return err;
}


// initializes the process list and their heaps
static void init_process_state_list(void){
    
    size_t i = 0;

    processes = (processes_list *)vmalloc(sizeof(processes_list));
    processes->count = 0;

    processes->state_list = (struct process_state_struct *)vmalloc(MAX_PROCESS_LIMIT*sizeof(process_state_struct));

    for(i = 0; i < MAX_PROCESS_LIMIT; i++){
        processes->state_list[i].pid = -1;
        processes->state_list[i].state = PROCESS_FILE_CLOSE;
        processes->state_list[i].lkm_heap = NULL;
    }

    return;
}


static err_t lkm_init(void){
    // create proc file
    struct proc_dir_entry *proc_file = proc_create("partb_2_17CS30011_17CS30026_20CS60R14", 0666, NULL, &file_ops);
    if(!proc_file)
        return -ENOENT;
    
    // register pointers
    file_ops.owner = THIS_MODULE;
    file_ops.open = file_open;
    file_ops.unlocked_ioctl = file_ioctl;
    file_ops.release = file_close;

    // process state array
    init_process_state_list();

    printk(KERN_ALERT "Initialized PB2-LKM\n");

    return 0;
}


static void lkm_exit(void){
    remove_proc_entry("partb_2_17CS30011_17CS30026_20CS60R14", NULL);

    vfree(processes);

    printk(KERN_ALERT "Removed PB2-LKM\n");
}


module_init(lkm_init);
module_exit(lkm_exit);
