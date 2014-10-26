FROM          quay.io/mozilla/gecko-base
MAINTAINER    Jonas Finnemann Jensen <jopsen@gmail.com>

ENV PATH $PATH:/home/worker/bin/

RUN sudo apt-get update
RUN sudo apt-get install -yq git-core

# Add utilities and configuration
COPY           mozconfigs/           /home/worker/mozconfigs/
RUN            mkdir -p              /home/worker/bin
COPY           build.sh              /home/worker/bin/build.sh
COPY           build-b2g-desktop.sh  /home/worker/bin/build-b2g-desktop.sh
COPY           build-git-b2g-desktop.sh  /home/worker/bin/build-git-b2g-desktop.sh
COPY           get_gaia_repo.js      /home/worker/bin/get_gaia_repo.js
COPY           get_gaia_revision.js  /home/worker/bin/get_gaia_revision.js
COPY           build-setup.sh        /home/worker/bin/build-setup.sh

# Git Utilities
ADD https://raw.githubusercontent.com/mozilla-b2g/gaia/master/build/docker/gaia-taskenv/git/git-ci-checkout-setup /home/worker/bin/git-ci-checkout-setup
ADD https://raw.githubusercontent.com/mozilla-b2g/gaia/master/build/docker/gaia-taskenv/git/git-ci-checkout-pr /home/worker/bin/git-ci-checkout-pr
ADD https://raw.githubusercontent.com/mozilla-b2g/gaia/master/build/docker/gaia-taskenv/git/git-ci-checkout-ref /home/worker/bin/git-ci-checkout-ref
RUN chmod u+x  /home/worker/bin/*

# Set a default command useful for debugging
CMD ["/bin/bash", "--login"]
