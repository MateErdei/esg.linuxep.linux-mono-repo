#!/bin/bash -e

set -x

#
# Script to automatically configure or update a Jenkins ubuntu node
#

THIS_DIR=$(readlink -f ${0%/*})

function update_apt()
{
    echo "=> Updating existing packages with apt..."
    apt-get update || return 1
    apt-get upgrade -qy || return 1
}

function update_yum()
{
    echo "=> Updating existing packages with yum..."
    if grep 'Red Hat Enterprise Linux release 8' /etc/redhat-release; then
        yum -y --nobest update || return 1
        ln -sf /usr/bin/python2.7 /usr/bin/python
    else
        yum -y update || return 1
    fi
}

if [ -n "$(which apt-get)" ]
then
    update_apt || ( rm -rf ./${THIS_DIR} && exit 1 )
elif [ -n "$(which yum)" ]
then
    update_yum || ( rm -rf ./${THIS_DIR} && exit 1 )
else
    echo System has neither apt-get or yum
    exit 1
fi

shutdown 1