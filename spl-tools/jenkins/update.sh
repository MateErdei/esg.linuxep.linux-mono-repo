#!/bin/bash -e

set -x

#
# Script to automatically configure or update a Jenkins ubuntu node
#

THIS_DIR=$(readlink -f ${0%/*})

touch /tmp/thisJenkinsJobMadeByViolaGang

function update_apt()
{
    echo "=> Updating existing packages with apt..."
    apt-get update || exit 1
    apt-get upgrade -qy || exit 1
}

function update_yum()
{
    echo "=> Updating existing packages with yum..."
    yum -y update || exit 1
}

if [ -n "$(which apt-get)" ]
then
    update_yum || rm -rf ./${THIS_DIR} && exit 1
elif [ -n "$(which yum)" ]
then
    update_yum || rm -rf ./${THIS_DIR} && exit 1
else
    exit 1
fi