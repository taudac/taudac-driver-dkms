export kernelver ?= $(shell uname -r)
export KERNELDIR ?= /lib/modules/${kernelver}
export DUMPDIR ?= ../../modules

GITHASH := $(shell git rev-parse HEAD)

DIRS = bcm codecs clk
BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
INSTALLDIRS = $(DIRS:%=install-%)
UNINSTALLDIRS = $(DIRS:%=uninstall-%)
DUMPDIRS = $(DIRS:%=modules_dump-%)

BfGreen='\033[1;32m'
NoColor='\033[0m'

all: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

clean: $(CLEANDIRS)
$(CLEANDIRS): 
	$(MAKE) -C $(@:clean-%=%) clean

install: $(INSTALLDIRS)
$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) install

dump: dump_prepare $(DUMPDIRS)
	echo $(BfGreen)"Modules copied to $(DUMPDIR)"$(NoColor)
$(DUMPDIRS):
	$(MAKE) -C $(@:modules_dump-%=%) dump

dump_prepare:
	@mkdir -p $(DUMPDIR)
	@echo $(GITHASH) > $(DUMPDIR)/taudac_git_hash

uninstall: $(UNINSTALLDIRS)
$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall

