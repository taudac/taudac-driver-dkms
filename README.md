TauDAC ASoC Driver
==================

The TauDAC driver is distributed **WITHOUT ANY WARRANTY** of any kind.
Use it at your own risk!

Copyright (C) 2017 Sergej Sawazki

Installation
------------

We will build and install the driver using DKMS.

### Preparation

If not already done, download and unzip the driver source directory:

    wget https://github.com/taudac/taudac-driver-dkms/archive/taudac-2.2.2.tar.gz
    tar -xzf taudac-2.2.2.tar.gz

Now copy the driver source to a directory where DKMS can find it:

    sudo cp -r taudac-driver-dkms-taudac-2.2.2 /usr/src/taudac-2.2.2

Install the build dependencies:

    sudo apt-get install dkms build-essential

### Installing the Kernel Headers

To build the driver, we will need to install the kernel headers. The way to do
it depends on the distribution you are using.

The **Raspbian** way:

    sudo apt-get install raspberrypi-kernel-headers

Please be patient, _"Unpacking raspberrypi-kernel-headers"_ might take an
hour or so...

The **Volumio** way:

    sudo volumio kernelsource

### Installing the driver

To build and install the driver using DKMS do:

    sudo dkms install -m taudac -v 2.2.2 --force

### Uninstalling the driver

The driver can be uninstalled using DKMS with:

    sudo dkms remove -m taudac -v 2.2.2 --all

Configuration
-------------

### Disabling the Raspberry Pi audio driver

The Raspberry Pi audio driver (`snd_bcm2835`) can be disabled by _commenting
out_ the following entry in `/boot/contig.txt`:

    dtparam=audio=on

References
----------

- [TauDAC homepage](http://www.taudac.com)
- [TauDAC git-repository](https://github.com/taudac/taudac-driver-dkms)
