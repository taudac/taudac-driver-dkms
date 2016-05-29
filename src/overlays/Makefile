PWD := $(shell pwd)

dtbo-y += taudac.dtbo

targets += $(dtbo-y)
always  := $(dtbo-y)

all:
	make -C /lib/modules/${kernelver}/build M=$(PWD)
	../../dkms.post_build

clean:
	make -C /lib/modules/${kernelver}/build M=$(PWD) clean

