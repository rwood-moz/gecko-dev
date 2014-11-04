#!/bin/bash -e

# Take care of any volume mounts created by a user other than worker
chown -R worker:worker /home/worker/

sudo -E -u worker "$@"
