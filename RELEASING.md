# Releasing
## Step 1: Create a Release on Github
1. Navigate to https://github.com/mattkae/miracle-wm/releases
2. Draft a new release
3. Name the tag `v.X.Y.Z`
4. Title the release `v.X.Y.Z`
5. Target a branch, most likely `develop`
6. Describe the release (You may generate release notes, but please make sure that they make sense before doing so)

## Step 2: Snap Release
1. Check out the commit that you just released in Step 1
2. Bump the version number in `snap/snapcraft.yaml` to the `X.Y.Z`
3. Commit this buumped version
4. Next (this part is silly), comment out the `override-pull` of the `miracle-wm` part
5. Run `snapcraft`
6. Finally, run `snapcraft upload --release=stable ./miracle-wm_*.snap`

TODO: Implement https://github.com/mattkae/miracle-wm/issues/59 to fix the weirdness of this process

## Step 3: Deb Release
1. Clone the repo (make sure that the folder is called `miracle-wm`)
2. Update `debian/changelog` with:
    - Version `X.Y.Z-distro` (where `distro` is "noble"")
    - The same content as you in the Github release
    - A correct current timestamp
3. Next:
```sh
cd miracle-wm
./tools/publish-ppa.sh <X.Y.Z> <DISTRO>
```
4. Navigate to https://launchpad.net/~matthew-kosarek/+archive/ubuntu/miracle-wm
5. Wait for the CI to finish, and the package should be ready

Note that you should rebuild for `mantic` and `noble`. Follow the instructions to make sure that it uploads.

## Step 4: RPM Release
Before following these steps, make sure that you've at least followed [this tutorial](https://www.redhat.com/sysadmin/create-rpm-package).

1. `fkinit`
2. `rpmdev-bumpspec -n 0.3.0 -c "Update to 0.3.0" miracle-wm.spec`
3. `rpmdev-spectool -g miracle-wm.spec`
4. `fedpkg new-sources miracle-wm-0.3.0.tar.gz`
5. `fedpkg ci -m "Update to 0.3.0"`
6. `fedpkg push && fedpkg build`
