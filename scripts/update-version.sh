#!/bin/bash
# Update version across all packaging files
# Usage: ./update-version.sh 2.6.0

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <new-version>"
    echo "Example: $0 2.6.0"
    exit 1
fi

NEW_VERSION="$1"
OLD_VERSION=$(grep PACKAGE_VERSION dkms.conf | cut -d'"' -f2)

echo "Updating version from $OLD_VERSION to $NEW_VERSION..."
echo

# Update dkms.conf
echo "Updating dkms.conf..."
sed -i "s/PACKAGE_VERSION=\"$OLD_VERSION\"/PACKAGE_VERSION=\"$NEW_VERSION\"/" dkms.conf

# Update debian/install
echo "Updating debian/install..."
sed -i "s/taudac-$OLD_VERSION/taudac-$NEW_VERSION/g" debian/install

# Update debian/postinst
echo "Updating debian/postinst..."
sed -i "s/PACKAGE_VERSION=$OLD_VERSION/PACKAGE_VERSION=$NEW_VERSION/" debian/postinst

# Update debian/prerm
echo "Updating debian/prerm..."
sed -i "s/PACKAGE_VERSION=$OLD_VERSION/PACKAGE_VERSION=$NEW_VERSION/" debian/prerm

# Prompt for debian/changelog update
echo
echo "Please update debian/changelog manually:"
echo "  dch -v ${NEW_VERSION}-1 \"New upstream release\""
echo
echo "Or edit debian/changelog directly to add:"
echo "---"
echo "taudac-dkms (${NEW_VERSION}-1) unstable; urgency=medium"
echo ""
echo "  * New upstream release ${NEW_VERSION}"
echo ""
echo " -- Sergej Sawazki <taudac@gmx.de>  $(date -R)"
echo "---"
echo
echo "Version updated to $NEW_VERSION in:"
echo "  - dkms.conf"
echo "  - debian/install"
echo "  - debian/postinst"
echo "  - debian/prerm"
echo
echo "Don't forget to:"
echo "  1. Update debian/changelog (manually or with 'dch -v ${NEW_VERSION}-1')"
echo "  2. Commit changes: git commit -am 'Bump version to $NEW_VERSION'"
echo "  3. Create tag: git tag -a v$NEW_VERSION -m 'Release version $NEW_VERSION'"
echo "  4. Push: git push && git push --tags"
