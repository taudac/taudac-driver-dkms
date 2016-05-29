PWD := $(shell pwd)

snd-soc-wm8741-objs := wm8741.o
obj-$(CONFIG_SND_SOC_WM8741)	+= snd-soc-wm8741.o

all:
	make -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	make -C /lib/modules/${kernelver}/build M=$(PWD) clean

