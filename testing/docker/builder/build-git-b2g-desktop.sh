#!/bin/bash -live

################################### build.sh ###################################

### Check that require variables are defined
test $GIT_REPOSITORY  # Should be an git repository url to pull from
test $GIT_REVISION    # Should be an git revision to pull down
test $GIT_BRANCH      # Should be an git branch to pull down
test $MOZCONFIG       # Should be a mozconfig file from mozconfig/ folder

gecko_git_dir=/home/worker/mozilla-central/source
gaia_dir=/home/worker/gaia/source

### Firefox Build Setup
# Clone mozilla-central
if [ ! -d "$gecko_git_dir" ];
then
  curl https://s3-us-west-2.amazonaws.com/test-caching/gecko-dev.tar.gz | tar xz
  mv gecko-dev $gecko_git_dir
fi

git ci-checkout-ref $gecko_git_dir $GIT_REPOSITORY $GIT_BRANCH $GIT_REVISION

# Create .mozbuild so mach doesn't complain about this
mkdir -p /home/worker/.mozbuild/

# Create object-folder exists
mkdir -p /home/worker/object-folder/

### Clone gaia in too
if [ ! -d "$gaia_dir" ];
then
  hg clone https://hg.mozilla.org/integration/gaia-central/ $gaia_dir
fi

### Pull and update gaia
cd /home/worker/gaia/source;
GAIA_REV=$(get_gaia_revision.js)
GAIA_REPO="https://hg.mozilla.org$(get_gaia_repo.js)"
hg pull -r $GAIA_REV $GAIA_REPO;
hg update $GAIA_REV;

cd /home/worker/mozilla-central/source;
./mach build;

### Make package
cd /home/worker/object-folder;
make package package-tests;

### Extract artifacts
# Navigate to dist/ folder
cd /home/worker/object-folder/dist;

ls -lah /home/worker/object-folder/dist/

# Target names are cached so make sure we discard them first if found.
rm -f target.linux-x86_64.tar.bz2 target.linux-x86_64.json target.tests.zip

# Artifacts folder is outside of the cache.
mkdir -p /home/worker/artifacts/

# Discard version numbers from packaged files, they just make it hard to write
# the right filename in the task payload where artifacts are declared
mv *.linux-x86_64.tar.bz2   /home/worker/artifacts/target.linux-x86_64.tar.bz2
mv *.linux-x86_64.json      /home/worker/artifacts/target.linux-x86_64.json
mv *.tests.zip              /home/worker/artifacts/target.tests.zip

################################### build.sh ###################################
