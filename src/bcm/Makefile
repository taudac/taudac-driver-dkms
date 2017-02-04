PWD := $(shell pwd)

snd-soc-taudac-objs := taudac.o
obj-m += snd-soc-taudac.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS_taudac.o := -DDEBUG
endif

all:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/${kernelver}/build M=$(PWD) clean

.PHONY: install
install:
	install -m 644 \
	snd-soc-taudac.ko /lib/modules/${kernelver}/kernel/sound/soc/bcm/
