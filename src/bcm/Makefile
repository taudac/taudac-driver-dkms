PWD := $(shell pwd)

INSTALLDIR = taudac

snd-soc-taudac-objs := taudac.o
obj-m += snd-soc-taudac.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_taudac.o := -DDEBUG
endif

all:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) modules

install: all
	$(MAKE) INSTALL_MOD_DIR=$(INSTALLDIR) \
		-C $(KERNELDIR)/build M=$(PWD) modules_install
		@depmod -a $(kernelver)

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

.PHONY: uninstall
uninstall:
	@rm -f $(KERNELDIR)/$(INSTALLDIR)/snd-soc-taudac.ko
	@depmod -a $(kernelver)

