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
    apt-get update || return 1
    apt-get upgrade -qy || return 1
    rm /home/jenkins/differentMachineId || return 1
    [[ ! -f /home/jenkins/differentMachineId ]] || return 1
    ls -l /home/jenkins
}

function update_yum()
{
    echo "=> Updating existing packages with yum..."
    yum -y update || return 1
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