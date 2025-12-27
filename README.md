TauDAC ASoC Driver
==================

The TauDAC driver is distributed **WITHOUT ANY WARRANTY** of any kind.
Use it at your own risk!

Copyright (C) 2017 Sergej Sawazki

Installation
------------

Before you start, define the TauDAC version variable:

    export taudacver=2.5.0

You can then use `$taudacver` in the commands below. Adjust the version if
you're installing a different release.

### Method 1: Debian Package (Recommended)

The easiest way to install the TauDAC driver is using the pre-built Debian package.

1. Download the latest `.deb` package from the [Releases page](https://github.com/taudac/taudac-driver-dkms/releases)

2. Install the package:

       sudo dpkg -i taudac-dkms_${taudacver}-1_all.deb

3. Verify installation:

       dkms status taudac

#### Uninstalling

    sudo apt-get remove taudac-dkms

### Method 2: Manual Installation using DKMS

If you prefer to install manually or the `.deb` package is not available for
your system, you can build and install the driver manually using DKMS.

#### Preparation

Download and unzip the driver source directory:

    wget https://github.com/taudac/taudac-driver-dkms/archive/taudac-${taudacver}.tar.gz
    tar -xzf taudac-${taudacver}.tar.gz

Copy the driver source to a directory where DKMS can find it:

    sudo cp -r taudac-driver-dkms-taudac-${taudacver} /usr/src/taudac-${taudacver}

Install the build dependencies:

    sudo apt-get install dkms build-essential

#### Installing the Kernel Headers

To build the driver, we need to install the kernel headers. The method depends
on your distribution.

**Raspbian / Raspberry Pi OS:**

    sudo apt-get install linux-headers-$(uname -r)

Please be patient, _"Unpacking raspberrypi-kernel-headers"_ might take a while...

**Volumio:**

    sudo volumio kernelsource

#### Building and Installing

Build and install the driver using DKMS:

    sudo dkms install -m taudac -v ${taudacver} --force

#### Uninstalling

Uninstall the driver using DKMS:

    sudo dkms remove -m taudac -v ${taudacver} --all

Configuration
-------------

### Disabling the Raspberry Pi audio driver

The Raspberry Pi audio driver (`snd_bcm2835`) can be disabled by _commenting
out_ the following entry in `/boot/config.txt`:

    dtparam=audio=on

References
----------

- [TauDAC homepage](http://www.taudac.com)
- [TauDAC git-repository](https://github.com/taudac/taudac-driver-dkms)
