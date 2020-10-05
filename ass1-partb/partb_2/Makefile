HEAP_DECL_PATH := $(PWD)/../heap

obj-m += lkm_2.o
lkm_2-y = lkm_pb2.o ../heap/heap.o
CFLAGS_lkm_pb2.o := -I$(HEAP_DECL_PATH)/ -I$(PWD)

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -f $(HEAP_DECL_PATH)/*.o $(HEAP_DECL_PATH)/.[!.]*
