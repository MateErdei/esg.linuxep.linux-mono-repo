#!/bin/bash

set -ex

if [[ -x $(which apt) ]]
then
    apt install -y nfs-kernel-server zip unzip
elif [[ -x $(which yum) ]]
then
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/CentOS-Base.repo
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/epel.repo
    cat /etc/yum.repos.d/CentOS-Base.repo
    cat /etc/yum.conf
    grep -r abn-centosrepo /etc/yum.repos.d/* /etc/yum.conf || true

    yum install -y "gcc" "gcc-c++" "make" nfs-utils zip
else
    echo "Can't find package management system"
    exit 1
fi
