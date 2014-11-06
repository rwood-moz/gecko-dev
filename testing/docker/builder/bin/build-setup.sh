#!/bin/bash -vex

gecko_dir=/home/worker/mozilla-central/source
gaia_dir=/home/worker/gaia/source

create_parent_dir() {
  parent_dir=$(dirname $1)
  if [ ! -d "$parent_dir" ]; then
    mkdir -p "$parent_dir"
  fi
}

### Firefox Build Setup
if [ ! -d "$gecko_dir" ]; then
  mkdir -p "$gecko_dir"
  # If repo exists on cache grab it from there, much faster, otherwise clone
  if ! tc-repo-cache.sh /home/worker/bin/repo-cache.json https://hg.mozilla.org/mozilla-central "$gecko_dir" ; then
    create_parent_dir $gecko_dir
    hg clone https://hg.mozilla.org/mozilla-central/ $gecko_dir
  fi
fi

# Create .mozbuild so mach doesn't complain about this
mkdir -p /home/worker/.mozbuild/

# Create object-folder exists
mkdir -p /home/worker/object-folder/
