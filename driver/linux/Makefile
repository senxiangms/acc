# Out-of-tree build (run on Linux with kernel headers):
#   make -C /lib/modules/$(uname -r)/build M=$(PWD) modules
#   sudo insmod aac.ko

obj-m += aac.o
aac-objs := aac_main.o

ccflags-y += -I$(src)/../../include
