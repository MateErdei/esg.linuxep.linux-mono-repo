#!/bin/bash
set -xe

#
# script to set an ubuntu template up for cloning
#

function delete_ID()
{
    rm /home/jenkins/differentMachineId || return 1
    [[ ! -f /home/jenkins/differentMachineId ]] || return 1
    ls -l /home/jenkins
}

if [ -n "$(which apt-get)" ]
then
    delete_ID || exit 1
fi

shutdown 1