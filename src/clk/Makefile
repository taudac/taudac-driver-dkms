PWD := $(shell pwd)

INSTALLDIR = taudac

obj-m += clk-gpio.o
obj-m += clk-si5351.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_clk-gpio.o := -DDEBUG
    CFLAGS_clk-si5351.o := -DDEBUG
endif

all:
	$(MAKE) -C $(KERNELDIR)/build  M=$(PWD) \
		KBUILD_EXTRA_SYMBOLS=$(PWD)/../lib/Module.symvers modules

install: all
	$(MAKE) INSTALL_MOD_DIR=$(INSTALLDIR) \
		-C $(KERNELDIR)/build M=$(PWD) modules_install
		@depmod -a $(kernelver)

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

.PHONY: uninstall
uninstall:
	@rm -f $(KERNELDIR)/$(INSTALLDIR)/clk-si5351.ko
	@depmod -a $(kernelver)

