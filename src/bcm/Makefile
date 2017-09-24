PWD := $(shell pwd)

ORIGDIR = sound/soc/bcm
INSTALLDIR = taudac
MODULENAME = snd-soc-taudac.ko

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

release: all
	@install -m 644 -vD -t ../$(RELEASEDIR)$(KERNELDIR)/kernel/$(ORIGDIR) \
		$(MODULENAME)

deb: all
	@install -m 644 -vD -t ../$(DEBTMPDIR)/$(DEBMODDIR) $(MODULENAME)

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

.PHONY: uninstall
uninstall:
	@rm -f $(KERNELDIR)/$(INSTALLDIR)/$(MODULENAME)
	@depmod -a $(kernelver)
