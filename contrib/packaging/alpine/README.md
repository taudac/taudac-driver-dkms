TauDAC Modules (Alpine package)
===============================

Builds the TauDAC ASoC driver as a signed Alpine `.apk` package, for
[myMPDos](https://github.com/jcorporation/myMPDos) and other Alpine aarch64
systems.

myMPDos prerequisites
---------------------

myMPDos's root filesystem is tmpfs, and kernel modules are normally served
from a read-only `modloop` squashfs image that gets wiped and re-linked on
every boot. Unless `/boot/cmdline.txt` adds `overlay` to `modules=` and sets
`overlay_size=<N>M` (e.g. `overlay_size=16M`), any module installed here is
silently lost on the next reboot. The package's `pre-install` hook checks
for this and refuses to install if it's missing, with instructions.

Before installing, add both to `/boot/cmdline.txt` and reboot, e.g.:

    modules=loop,squashfs,sd-mod,usb-storage,overlay overlay_size=16M quiet console=tty1

Then confirm with `mount | grep /lib/modules` that it shows `type overlay`.

Building locally
----------------

The build script (`build.sh`) auto-detects whether it's running on a genuine
Alpine aarch64 host: if so, it runs `abuild -r` directly there. Otherwise, it
builds inside a Docker container instead, needing the one-time setup below.

### 1. One-time setup (Docker builds only)

Install Docker if you don't already have it (`docker --version` to check):

    sudo apt-get update
    sudo apt-get install -y docker.io
    sudo usermod -aG docker "$USER"

Log out and back in (or `newgrp docker`) for the group membership to take
effect -- otherwise every `docker` command needs `sudo` in front of it.

`build.sh` registers QEMU for `--platform linux/arm64` automatically when
needed. Not needed on an aarch64 machine/CI runner -- the container already
matches the host architecture there, so nothing is emulated.

### 2. Build

Launch the build script:

    ./build.sh

With no `TARGET_KERNEL` set, this auto-resolves whatever `linux-rpi-dev`
is currently latest on Alpine's latest-stable branch for a Docker build,
or whatever's actually running on a native build. To pin an exact kernel
version, set `TARGET_KERNEL`, for example:

    TARGET_KERNEL=6.18.35-r0 ./build.sh

Once the package is built, you will find it in:

    ./dist/

Installing
----------

On the Alpine Raspberry Pi host, install the package using:

    apk add --allow-untrusted ./dist/taudac-modules-rpi-*.apk
    lbu commit
    reboot

`lbu commit` makes the package survive the next boot.

### Uninstalling

    apk del taudac-modules-rpi
    lbu commit

This does not itself revert the additions to `cmdline.txt` -- remove that
by hand if needed.

Signing
-------

Container builds only -- a native build uses your own existing `abuild`
signing setup (`abuild-keygen`), untouched by `build.sh`.

With no signing key provided, `build.sh` generates a fresh local-only key
on every run -- fine for testing, not for publishing a real release. To use
a real, stable signing key (e.g. in CI, from a secret), point
`ABUILD_KEY_DIR` at a directory containing a `<name>.rsa`/`<name>.rsa.pub`
keypair:

    ABUILD_KEY_DIR=/path/to/keydir ./build.sh
