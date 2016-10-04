PWD := $(shell pwd)

obj-m += clk-gpio.o
obj-m += clk-si5351.o

all:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) clean

