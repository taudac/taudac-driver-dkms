#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Prints the linux-rpi-dev version(s) needing a TARGET_KERNEL build. Today
# that's just the latest Alpine version string (e.g. "6.18.35-r0"), since
# there's no published apk repo yet to check against.

set -e

# "latest" tracks Alpine's latest-stable branch, same default as
# build.sh/Dockerfile's ALPINE_BRANCH -- so this keeps following new Alpine
# releases indefinitely instead of going stale once a pinned branch reaches
# EOL. Alpine's CDN calls that branch "latest-stable"; a pinned branch (e.g.
# ALPINE_BRANCH=3.24 or v3.24) maps to its "vNN.NN" directory instead.
ALPINE_BRANCH=${ALPINE_BRANCH:-latest}
ALPINE_ARCH=aarch64
ALPINE_MIRROR=https://dl-cdn.alpinelinux.org/alpine

case "$ALPINE_BRANCH" in
	latest) branch_path=latest-stable ;;
	*) branch_path="v${ALPINE_BRANCH#v}" ;;
esac

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

curl -sf -o "$work/APKINDEX.tar.gz" \
    "$ALPINE_MIRROR/$branch_path/main/$ALPINE_ARCH/APKINDEX.tar.gz"
tar -xzf "$work/APKINDEX.tar.gz" -C "$work" APKINDEX

ver=$(awk '/^P:linux-rpi-dev$/{f=1} f&&/^V:/{print substr($0,3); exit}' "$work/APKINDEX")
[ -n "$ver" ] || { echo "Could not find linux-rpi-dev in $branch_path APKINDEX" >&2; exit 1; }

echo "$ver"
