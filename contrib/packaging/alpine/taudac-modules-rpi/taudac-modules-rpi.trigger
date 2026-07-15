#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Run by apk whenever /lib/modules changes (see triggers= in APKBUILD),
# not just on this package's own install/upgrade -- so removal and any
# other package's changes to /lib/modules also get depmod'd, in one
# batched run per transaction. Same idiom as Alpine's own mkinitfs.trigger.

depmod -a "$(uname -r)" || echo "TauDAC modules: depmod failed, kernel module dependencies may be stale" >&2
