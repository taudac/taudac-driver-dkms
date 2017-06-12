PWD := $(shell pwd)

INSTALLDIR = /lib/modules/${kernelver}/kernel/drivers/clk

obj-m += clk-si5351.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_clk-si5351.o := -DDEBUG
endif

all:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONY: install uninstall
install:
	install -m 644 clk-si5351.ko $(INSTALLDIR)/

uninstall:
	rm $(INSTALLDIR)/clk-si5351.ko

