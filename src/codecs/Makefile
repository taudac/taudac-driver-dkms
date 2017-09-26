PWD := $(shell pwd)

ORIGDIR = sound/soc/codecs
INSTALLDIR = taudac
MODULENAME = snd-soc-wm8741.ko

snd-soc-wm8741-objs := wm8741.o
obj-m += snd-soc-wm8741.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_wm8741.o := -DDEBUG
endif

all:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) modules

install: all
	$(MAKE) INSTALL_MOD_DIR=$(INSTALLDIR) \
		-C $(KERNELDIR)/build M=$(PWD) modules_install
		@depmod -a $(kernelver)

release: all
	@install -m 644 -vD -t $(PACKAGEDIR)$(KERNELDIR)/kernel/$(ORIGDIR) \
		$(MODULENAME)

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

.PHONY: uninstall
uninstall:
	@rm -f $(KERNELDIR)/$(INSTALLDIR)/$(MODULENAME)
	@depmod -a $(kernelver)
