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

cgreen = '\033[1;32m'
cred   = '\033[1;31m'
cend   = '\033[0m'

define ok
	@echo $(cgreen)$(1)$(cend)
endef

define die
	(echo $(cred)$(1)$(cend); exit 1)
endef

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
	$(call ok,"Modules copied to $(RELEASEDIR)")
$(RELEASEDIRS):
	$(MAKE) -C $(@:release-%=%) release

release_prepare:
	@git describe --exact-match HEAD > /dev/null || \
		$(call die,"HEAD is not tagged!")
	@mkdir -p $(RELEASEDIR)
	@echo $(GITHASH) > $(RELEASEDIR)/taudac_git_hash

uninstall: $(UNINSTALLDIRS)
$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall

