#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Prints the apk version of whichever package owns /lib/modules/$(uname -r)
# -- the kernel actually running right now.

set -e

db=/lib/apk/db/installed
[ -f "$db" ] || { echo "Not an Alpine system (no $db)" >&2; exit 1; }

target="lib/modules/$(uname -r)"
ver=$(awk -v target="$target" '
    /^V:/ { ver = substr($0, 3) }
    $0 == "F:" target { print ver; exit }
' "$db")

[ -n "$ver" ] || { echo "No installed package owns /$target" >&2; exit 1; }

echo "$ver"
