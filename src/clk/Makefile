PWD := $(shell pwd)

obj-m += clk-si5351.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_clk-si5351.o := -DDEBUG
endif

all:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) clean

.PHONY: install
install:
	install -m 644 \
	clk-si5351.ko /lib/modules/${kernelver}/kernel/drivers/clk/
