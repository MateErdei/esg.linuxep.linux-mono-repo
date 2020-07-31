#!/bin/bash

set -ex

if [[ -x $(which apt) ]]
then
    apt install -y python3.7-dev nfs-kernel-server
elif [[ -x $(which yum) ]]
then
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/CentOS-Base.repo
    cat /etc/yum.repos.d/CentOS-Base.repo
    yum install -y "gcc" "gcc-c++" "make" "capnproto-devel" "capnproto-libs" "capnproto" nfs-utils
else
    echo "Can't find package management system"
    exit 1
fi
