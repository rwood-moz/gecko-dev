#!/bin/bash -ve

############################### system-setup.sh ###############################

home="/home/worker"

mkdir -p $home/bin
mkdir -p $home/tools
mkdir -p $home/tools/tooltool_cache

wget -O $home/tools/tooltool.py https://raw.githubusercontent.com/mozilla/build-tooltool/master/tooltool.py

# Remove the setup.sh setup, we don't really need this script anymore, deleting
# it keeps the image as clean as possible.
rm $0; echo "Deleted $0";

