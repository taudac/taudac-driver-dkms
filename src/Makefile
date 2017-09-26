export kernelver ?= $(shell uname -r)
export KERNELDIR ?= /lib/modules/${kernelver}

export KERNELDIR  ?= /lib/modules/${kernelver}
export RELEASEDIR ?= $(realpath ../..)/modules

GITHASH := $(shell git rev-parse HEAD)
GITTAG  := $(shell git describe --tags)

DIRS = bcm codecs clk
BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
INSTALLDIRS = $(DIRS:%=install-%)
UNINSTALLDIRS = $(DIRS:%=uninstall-%)
RELEASEDIRS = $(DIRS:%=release-%)

cred = '\033[1;31m'
cgrn = '\033[1;32m'
cylw = '\033[1;33m'
cend = '\033[0m'

define ok
	echo $(cgrn)$(1)$(cend)
endef

define warn
	echo $(cylw)$(1)$(cend)
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

package: $(RELEASEDIRS)

release: release_prepare $(RELEASEDIRS)
	@echo $(GITHASH) > $(RELEASEDIR)/taudac.modules.hash
	@echo "#Bump to $(GITTAG)" > $(RELEASEDIR)/.git/taudac_git_tag
	@$(call ok,"[$(GITTAG)] Modules copied to $(realpath $(RELEASEDIR))")
$(RELEASEDIRS):
	$(MAKE) -C $(@:release-%=%) release

release_prepare:
	@git describe --exact-match HEAD > /dev/null || \
		$(call warn,"HEAD is not tagged!")
	@mkdir -p $(RELEASEDIR)

uninstall: $(UNINSTALLDIRS)
$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall
