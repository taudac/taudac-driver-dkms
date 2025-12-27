# Release Checklist for TauDAC DKMS

Use this checklist when preparing a new release.

## Pre-Release

- [ ] All changes committed and pushed to main branch
- [ ] Code tested on target systems (Raspberry Pi, kernel 6.12+)
- [ ] No outstanding issues that should block the release
- [ ] Version decided (e.g., 2.6.0)

## Version Update

- [ ] Run `./update-version.sh X.Y.Z` to update version strings
- [ ] Update `debian/changelog` manually or with `dch -v X.Y.Z-1 "Release notes"`
- [ ] Review changes in dkms.conf, debian/install, debian/postinst, debian/prerm
- [ ] Commit version changes: `git commit -am "Bump version to X.Y.Z"`

## Local Testing

- [ ] Build package locally: `./build-deb.sh`
- [ ] Verify .deb file created: `ls -lh ../taudac-dkms_*.deb`
- [ ] Test installation: `sudo dpkg -i ../taudac-dkms_*.deb`
- [ ] Verify DKMS status: `dkms status taudac`
- [ ] Check modules load: `modinfo snd-soc-taudac`
- [ ] Test uninstallation: `sudo apt-get remove taudac-dkms`
- [ ] Clean up: `sudo apt-get autoremove`

## GitHub Release

- [ ] Push version commit: `git push origin main`
- [ ] Create annotated tag: `git tag -a vX.Y.Z -m "Release version X.Y.Z"`
- [ ] Push tag: `git push origin vX.Y.Z`
- [ ] Monitor GitHub Actions workflow: Check Actions tab in GitHub
- [ ] Verify workflow completes successfully (build + test jobs)
- [ ] Check GitHub Release was created automatically
- [ ] Verify .deb file is attached to the release

## Post-Release

- [ ] Download .deb from GitHub Release and test installation
- [ ] Update any external documentation that references version numbers
- [ ] Announce release (website, mailing list, forums, etc.)
- [ ] Update taudac.com website if needed
- [ ] Monitor issue tracker for any installation problems

## If Something Goes Wrong

### Build Fails Locally
1. Check build dependencies: `sudo apt-get install build-essential devscripts debhelper dkms`
2. Review build log for specific errors
3. Check file permissions on debian/ scripts

### GitHub Actions Fails
1. Check workflow logs in GitHub Actions tab
2. Verify all required files are committed
3. Check syntax of .github/workflows/build-deb.yml
4. Ensure tag was pushed (not just created locally)

### DKMS Build Fails
1. Verify kernel version compatibility (6.12+): `uname -r`
2. Check kernel headers installed: `dpkg -l | grep linux-headers`
3. Review DKMS build log: `cat /var/lib/dkms/taudac/X.Y.Z/build/make.log`

### Need to Fix Release
1. Delete the tag locally: `git tag -d vX.Y.Z`
2. Delete the tag remotely: `git push origin :refs/tags/vX.Y.Z`
3. Delete the GitHub Release through web UI
4. Fix the issues
5. Re-create tag and push

## Version Numbering

Follow semantic versioning (MAJOR.MINOR.PATCH):

- **MAJOR**: Incompatible API changes, major kernel version bumps
- **MINOR**: New features, backwards-compatible changes
- **PATCH**: Bug fixes, documentation updates

Examples:
- 2.5.0 → 2.5.1 (bug fix)
- 2.5.1 → 2.6.0 (new feature)
- 2.6.0 → 3.0.0 (breaking change)

## Debian Package Revision

The `-1` in `X.Y.Z-1` is the Debian package revision:
- Increment for packaging-only changes
- Reset to `-1` for new upstream versions

Examples:
- 2.5.0-1 → 2.5.0-2 (fixed debian/postinst bug)
- 2.5.0-2 → 2.6.0-1 (new upstream release)
