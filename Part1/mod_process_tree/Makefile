obj-m += process_tree.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
test:
	dmesg -C
	insmod process_tree.ko
	rmmod process_tree.ko
	dmesg
