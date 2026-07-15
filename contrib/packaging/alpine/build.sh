#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds a signed taudac-modules-rpi .apk: runs `abuild -r`. On a genuine
# Alpine aarch64 host, runs it directly, using your own existing abuild
# signing setup. Otherwise, inside a genuinely aarch64 container -- so
# build() still compiles natively, no cross-compiler -- generating a
# throwaway signing key unless ABUILD_KEY_DIR says otherwise. The
# in-container steps (builder account, signing key, abuild -r) are in
# entrypoint.sh.
#
# Usage: build.sh
#        TARGET_KERNEL=6.18.35-r0 build.sh
#
# Env:
#   TARGET_KERNEL   exact linux-rpi version (default: latest)
#   ALPINE_BRANCH   Alpine branch for the builder image, container mode
#                   only (default: latest)
#   ABUILD_KEY_DIR  dir with an existing <name>.rsa/<name>.rsa.pub signing
#                   keypair, container mode only (default: generate a
#                   fresh local-only key)

set -e

if [ "$(id -u)" -eq 0 ]; then
    echo "ERROR: build.sh must not be run as root." >&2
    echo "       Give your user Docker access instead (in CI too)." >&2
    exit 1
fi

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ -f /etc/alpine-release ] && [ "$(uname -m)" = aarch64 ]; then
    # --- Native build: genuine Alpine aarch64 host ---
    echo "Genuine Alpine aarch64 host, running abuild -r directly"
    if [ -z "$TARGET_KERNEL" ]; then
        echo "TARGET_KERNEL not set, using the running kernel"
        TARGET_KERNEL=$("$here/running-kernel.sh")
    fi
    echo "Building TauDAC modules for linux-rpi ${TARGET_KERNEL}"
    export TARGET_KERNEL
    (cd "$here/taudac-modules-rpi" && abuild -r)

    packages_dir="$HOME/packages"
else
    # --- Docker build: everything else ---
    # "latest" avoids going stale once a pinned branch reaches EOL; accepts a
    # leading "v" too (Alpine's own branch-naming convention) since Docker tags
    # don't use it.
    ALPINE_BRANCH=${ALPINE_BRANCH:-latest}
    ALPINE_BRANCH=${ALPINE_BRANCH#v}
    repo_root=$(CDPATH= cd -- "$here/../../.." && pwd)
    image="taudac-abuild:${ALPINE_BRANCH}"

    # binfmt registration doesn't survive a reboot, so check every run.
    if [ "$(uname -m)" != aarch64 ] && [ ! -e /proc/sys/fs/binfmt_misc/qemu-aarch64 ]; then
        echo "Registering QEMU aarch64 emulation"
        docker run --privileged --rm tonistiigi/binfmt --install arm64 >/dev/null
    fi

    echo "Building builder image"
    # --pull: without it, a stale cached alpine:latest layer would stick around
    # instead of picking up new Alpine releases.
    docker build -q --pull --platform linux/arm64 -t "$image" \
        --build-arg "ALPINE_BRANCH=${ALPINE_BRANCH}" "$here" >/dev/null

    home=$(mktemp -d)
    trap 'rm -rf "$home"' EXIT
    mkdir -p "$home/.abuild"

    if [ -n "$ABUILD_KEY_DIR" ]; then
        echo "Using signing key from \$ABUILD_KEY_DIR"
        cp "$ABUILD_KEY_DIR"/*.rsa "$ABUILD_KEY_DIR"/*.rsa.pub "$home/.abuild/"
        key=$(cd "$ABUILD_KEY_DIR" && ls ./*.rsa)
        echo "PACKAGER_PRIVKEY=\"/home/build/.abuild/${key#./}\"" > "$home/.abuild/abuild.conf"
    fi

    if [ -z "$TARGET_KERNEL" ]; then
        echo "TARGET_KERNEL not set, resolving latest linux-rpi-dev"
        TARGET_KERNEL=$("$here/pending-kernels.sh")
    fi

    echo "Building TauDAC modules for linux-rpi ${TARGET_KERNEL}"
    docker run --rm \
        --platform linux/arm64 \
        -e HOME=/home/build \
        -e TARGET_KERNEL="$TARGET_KERNEL" \
        -e HOST_UID="$(id -u)" \
        -v "$repo_root:/repo" \
        -v "$home:/home/build" \
        -w /repo/contrib/packaging/alpine \
        "$image" ./entrypoint.sh

    packages_dir="$home/packages"
fi

echo "Collecting built .apk"
mkdir -p "$here/dist"
find "$packages_dir" -name '*.apk' -exec cp -v {} "$here/dist/" \;

echo "Done: $here/dist/"
