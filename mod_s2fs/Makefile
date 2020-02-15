obj-m += s2fs.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo umount ./mnt
	sudo rmmod s2fs
	sudo rm -rf mnt
test:
	sudo mkdir mnt
	sudo insmod s2fs.ko
	sudo mount -t s2fs nodev mnt
	cat mnt/foo/bar
