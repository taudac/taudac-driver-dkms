#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Runs inside the builder container (see build.sh) as root: creates a
# "builder" account matching the host's own uid (so bind-mounted output
# comes out owned by you, not some arbitrary container uid), adds it to
# the abuild group, then drops to it for abuild -r. Needs a real named
# account, not a bare uid: abuild-sudo looks up group membership by name
# and fails against an unmapped uid.
#
# Env (set by build.sh via docker run -e): HOST_UID, HOME, TARGET_KERNEL

set -e

# Dockerfile build discarded the package index (--no-cache) -- refresh it
# so abuild can resolve makedepends="linux-rpi-dev=...".
apk update -q
adduser -D -u "$HOST_UID" -h "$HOME" builder
addgroup builder abuild
chown -R builder "$HOME"

# Key generated as builder (correct ownership under $HOME/.abuild), but
# installing the *public* half into /etc/apk/keys needs root -- otherwise
# abuild refuses to trust the package it just signed when building the
# repo index.
su builder -c "ls \$HOME/.abuild/*.rsa >/dev/null 2>&1 || abuild-keygen -a -n"
cp "$HOME"/.abuild/*.rsa.pub /etc/apk/keys/

# Build the package.
su builder -c "cd taudac-modules-rpi && abuild -r"
