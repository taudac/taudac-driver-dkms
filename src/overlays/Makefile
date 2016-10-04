PWD := $(shell pwd)

dtbo-y += taudac.dtbo

targets += $(dtbo-y)
always  := $(dtbo-y)

all:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD)
	../../dkms.post_build

clean:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) clean

