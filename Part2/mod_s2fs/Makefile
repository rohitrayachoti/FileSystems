obj-m += s2fs.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
test:
	insmod s2fs.ko
	mount -t s2fs nodev mnt
	mount
	umount ./mnt
	mount
	rmmod s2fs.ko
