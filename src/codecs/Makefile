PWD := $(shell pwd)

snd-soc-wm8741-objs := wm8741.o
obj-m += snd-soc-wm8741.o

all:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) clean

