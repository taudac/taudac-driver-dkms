PWD := $(shell pwd)

INSTALLDIR = taudac

obj-m += rational.o

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

