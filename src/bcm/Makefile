PWD := $(shell pwd)

snd-soc-taudac-objs := taudac.o
obj-m += snd-soc-taudac.o

all:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) clean

