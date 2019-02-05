export kernelver ?= $(shell uname -r)

export KERNELDIR  ?= $(prefix)/lib/modules/${kernelver}
export PACKAGEDIR ?= ../../modules
export MODULESDIR ?= lib/modules/${kernelver}

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

$(DIRS): $(BUILDDIRS)

all: $(BUILDDIRS)

install: $(INSTALLDIRS)

uninstall: $(UNINSTALLDIRS)

package: export RELEASEDIR := $(realpath $(PACKAGEDIR))/$(MODULESDIR)
package: $(RELEASEDIRS)

_release_prepare:
	@mkdir -pv $(PACKAGEDIR)/.git
	@git describe --exact-match HEAD > /dev/null || \
		$(call warn,"HEAD is not tagged!")

release: package _release_prepare
	$(eval GITHASH := $(shell git rev-parse HEAD))
	$(eval GITTAG := $(shell git describe --tags))
	$(eval KERNELRELEASE := $(shell echo $(kernelver) | sed -r 's/([0-9\.]+).*/\1/'))
	@echo $(GITHASH) > $(PACKAGEDIR)/taudac.modules.hash
	@echo "#$(GITTAG) for $(KERNELRELEASE)" > $(PACKAGEDIR)/.git/taudac_git_tag
	@$(call ok,"[$(GITTAG)] Modules copied to $(realpath $(PACKAGEDIR))")

clean: $(CLEANDIRS)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) install

$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall

$(RELEASEDIRS):
	$(MAKE) -C $(@:release-%=%) release

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean
