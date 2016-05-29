PWD := $(shell pwd)

obj-m += clk-gpio.o
obj-m += clk-si5351.o

all:
	make -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	make -C /lib/modules/${kernelver}/build M=$(PWD) clean

