#!/bin/bash -e

#
# Script to automatically configure or update a Jenkins ubuntu node
#

THIS_DIR=$(readlink -f ${0%/*})

touch /tmp/thisJenkinsJobMadeByViolaGang

echo "=> Updating existing packages..."
apt-get update
apt-get dist-upgrade -qy