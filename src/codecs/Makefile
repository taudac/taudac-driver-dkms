PWD := $(shell pwd)

ifeq ($(INSTALL_TO_ORIGDIR), 1)
    INSTALLDIR = kernel/sound/soc/codecs
else
    INSTALLDIR = taudac
endif

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
	@install -m 644 -vD -t $(RELEASEDIR)/$(INSTALLDIR) $(MODULENAME)

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

.PHONY: uninstall
uninstall:
	@rm -f $(KERNELDIR)/$(INSTALLDIR)/$(MODULENAME)
	@depmod -a $(kernelver)
