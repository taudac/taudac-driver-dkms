PWD := $(shell pwd)

ifeq ($(INSTALL_TO_ORIGDIR), 1)
    INSTALLDIR = kernel/sound/soc/bcm
else
    INSTALLDIR = taudac
endif

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
	@install -m 644 -vD -t $(RELEASEDIR)/$(INSTALLDIR) $(MODULENAME)

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

.PHONY: uninstall
uninstall:
	@rm -f $(KERNELDIR)/$(INSTALLDIR)/$(MODULENAME)
	@depmod -a $(kernelver)
