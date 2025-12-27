#!/bin/bash
# Build script for TauDAC DKMS Debian package

set -e

echo "Building TauDAC DKMS Debian package..."
echo "======================================="
echo

# Check if we're in the right directory
if [ ! -f "dkms.conf" ]; then
    echo "Error: dkms.conf not found. Please run this script from the repository root."
    exit 1
fi

# Check for required tools
if ! command -v dpkg-buildpackage &> /dev/null; then
    echo "Error: dpkg-buildpackage not found. Please install: sudo apt-get install build-essential devscripts"
    exit 1
fi

if ! command -v dh_dkms &> /dev/null; then
    echo "Error: dh_dkms not found. Please install: sudo apt-get install dkms debhelper"
    exit 1
fi

# Clean any previous builds
echo "Cleaning previous builds..."
rm -rf debian/.debhelper
rm -f debian/debhelper-build-stamp
rm -f debian/files
rm -f debian/*.substvars
rm -f debian/*.log
rm -rf debian/taudac-dkms/

# Build the package
echo "Building package..."
dpkg-buildpackage -us -uc -b

# Collect artifacts into dist/ inside the repo for convenience
OUTDIR=dist
mkdir -p "$OUTDIR"
mv ../taudac-dkms*.deb "$OUTDIR" 2>/dev/null || true
mv ../taudac-dkms*.buildinfo "$OUTDIR" 2>/dev/null || true
mv ../taudac-dkms*.changes "$OUTDIR" 2>/dev/null || true
mv ../taudac-dkms*.dsc "$OUTDIR" 2>/dev/null || true
mv ../taudac-dkms*.tar.* "$OUTDIR" 2>/dev/null || true

# Show results
echo
echo "======================================="
echo "Build complete!"
echo "======================================="
echo
echo "Packages created:"
ls -lh "$OUTDIR"/*.deb 2>/dev/null || echo "No .deb files found in $OUTDIR"
echo
echo "To install: sudo dpkg -i $OUTDIR/taudac-dkms_*.deb"
echo "Then fix dependencies if needed: sudo apt-get install -f"
