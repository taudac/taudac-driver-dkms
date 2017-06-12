PWD := $(shell pwd)

INSTALLDIR = /lib/modules/${kernelver}/kernel/sound/soc/codecs

snd-soc-wm8741-objs := wm8741.o
obj-m += snd-soc-wm8741.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_wm8741.o := -DDEBUG
endif

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONY: install uninstall
install:
	install -m 644 snd-soc-wm8741.ko $(INSTALLDIR)/

uninstall:
	rm $(INSTALLDIR)/snd-soc-wm8741.ko

