#!/bin/bash -e

#
# Script to automatically configure or update a Jenkins ubuntu node
#

THIS_DIR=$(readlink -f ${0%/*})

touch /tmp/thisJenkinsJobMadeByViolaGang

function update_apt()
{
    echo "=> Updating existing packages with apt..."
    apt-get update
    apt-get dist-upgrade -qy
}

function update_yum()
{
    echo "=> Updating existing packages with yum..."
    yum -y update
}

which apt-get &>/dev/null && update_apt || update_yum