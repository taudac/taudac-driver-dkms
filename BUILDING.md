# Building Debian Package

This document describes how to build the TauDAC DKMS Debian package.

## Prerequisites

Install the required build tools:

```bash
sudo apt-get update
sudo apt-get install build-essential devscripts debhelper dkms
```

## Building Locally

### Option 1: Using the build script

```bash
chmod +x build-deb.sh
./build-deb.sh
```

### Option 2: Manual build

```bash
dpkg-buildpackage -us -uc -b
```

The `.deb` package will be created in the parent directory.

## Installing the Package

```bash
sudo dpkg -i ../taudac-dkms_2.5.0-1_all.deb
sudo apt-get install -f  # Fix any missing dependencies
```

## Verifying Installation

Check if DKMS registered the module:

```bash
dkms status taudac
```

Check if kernel modules are available:

```bash
modinfo snd-soc-taudac
modinfo snd-soc-wm8741
modinfo clk-si5351
```

## Uninstalling

```bash
sudo apt-get remove taudac-dkms
```

## Automated Builds (GitHub Actions)

The package is automatically built and released on GitHub when you push a version tag:

```bash
git tag -a v2.5.0 -m "Release version 2.5.0"
git push origin v2.5.0
```

The workflow will:
1. Build the `.deb` package
2. Test installation in a clean Ubuntu environment
3. Create a GitHub Release with the `.deb` file attached

Users can then download the `.deb` directly from the GitHub Releases page.

## Package Contents

The package installs:
- Source files to `/usr/src/taudac-2.5.0/`
- DKMS configuration
- Automatically builds modules for installed kernels

## Troubleshooting

### Build fails with "debhelper-compat (= 13)" error

Install newer debhelper:
```bash
sudo apt-get install debhelper-compat
```

Or edit `debian/control` to use an older version like `debhelper (>= 12)`.

### DKMS build fails

Check kernel headers are installed:
```bash
sudo apt-get install linux-headers-$(uname -r)
```

Or for Raspbian:
```bash
sudo apt-get install raspberrypi-kernel-headers
```

### Module doesn't load

Check kernel version compatibility (requires 6.12+):
```bash
uname -r
```
