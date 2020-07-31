#!/bin/bash

set -ex

if [[ -x $(which apt) ]]
then
    apt install -y python3.7-dev nfs-kernel-server
elif [[ -x $(which yum) ]]
then
    ping abn-centosrepo
    ping abn-engrepo.eng.sophos
#    sed -i -e's/abn-engrepo.eng.sophos/abn-centosrepo/g' /etc/yum.repos.d/CentOS-Base.repo
    cat /etc/yum.repos.d/CentOS-Base.repo
    yum install -y "gcc" "gcc-c++" "make" "capnproto-devel" "capnproto-libs" "capnproto" nfs-utils
else
    echo "Can't find package management system"
    exit 1
fi
