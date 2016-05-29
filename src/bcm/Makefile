PWD := $(shell pwd)

snd-soc-taudac-objs := taudac.o
obj-m += snd-soc-taudac.o

all:
	make -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	make -C /lib/modules/${kernelver}/build M=$(PWD) clean

