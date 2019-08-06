#!/bin/bash -e

set -x

function deleteID()
{
    rm /home/jenkins/differentMachineId || return 1
    [[ ! -f /home/jenkins/differentMachineId ]] || return 1
    ls -l /home/jenkins
}

if [ -n "$(which apt-get)" ]
then
    deleteID || exit 1
fi

shutdown now