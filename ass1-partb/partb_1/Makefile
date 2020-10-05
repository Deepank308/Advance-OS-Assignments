HEAP_DECL_PATH := $(PWD)/../heap

obj-m += lkm_meow.o
lkm_meow-y = lkm_pb1.o ../heap/heap.o
CFLAGS_lkm_pb1.o := -I$(HEAP_DECL_PATH)/

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -f $(HEAP_DECL_PATH)/*.o $(HEAP_DECL_PATH)/.[!.]*
