export kernelver ?= $(shell uname -r)

export KERNELDIR  ?= /lib/modules/${kernelver}
export RELEASEDIR ?= ../../modules

GITHASH := $(shell git rev-parse HEAD)
GITTAG  := $(shell git describe --tags)

DIRS = bcm codecs clk
BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
INSTALLDIRS = $(DIRS:%=install-%)
UNINSTALLDIRS = $(DIRS:%=uninstall-%)
RELEASEDIRS = $(DIRS:%=release-%)

BfGreen = '\033[1;32m'
BfRed   = '\033[1;31m'
NoColor = '\033[0m'

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

release: release_prepare $(RELEASEDIRS)
	@git -C $(RELEASEDIR) commit -am "Bump to $(GITTAG)"
	@echo $(BfGreen)"Modules copied to $(RELEASEDIR)"$(NoColor)
$(RELEASEDIRS):
	$(MAKE) -C $(@:release-%=%) release

release_prepare:
	@git describe --exact-match HEAD || \
		(echo $(BfRed)"HEAD is not tagged!"$(NoColor); exit 1;)
	@mkdir -p $(RELEASEDIR)
	@echo $(GITHASH) > $(RELEASEDIR)/taudac_git_hash

uninstall: $(UNINSTALLDIRS)
$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall

