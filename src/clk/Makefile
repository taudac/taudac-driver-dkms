PWD := $(shell pwd)

ORIGDIR = drivers/clk
INSTALLDIR = taudac
MODULENAME = clk-si5351.ko

obj-m += clk-si5351.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_clk-si5351.o := -DDEBUG
endif

all:
	$(MAKE) -C $(KERNELDIR)/build  M=$(PWD) modules

install: all
	$(MAKE) INSTALL_MOD_DIR=$(INSTALLDIR) \
		-C $(KERNELDIR)/build M=$(PWD) modules_install
		@depmod -a $(kernelver)

dump: all
	@install -m 644 -D -t ../$(DUMPDIR)/$(KERNELDIR)/kernel/$(ORIGDIR) \
		$(MODULENAME)

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

.PHONY: uninstall
uninstall:
	@rm -f $(KERNELDIR)/$(INSTALLDIR)/$(MODULENAME)
	@depmod -a $(kernelver)

