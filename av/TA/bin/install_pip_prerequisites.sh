#!/bin/bash

set -ex

if [[ -x $(which apt-get) ]]
then
    if [[ $(lsb_release -rs) == "18.04" ]]; then
        apt-get install -y python3.7 python3.7-dev
    else
        apt-get install -y python3.8 python3.8-dev
    fi
    apt-get install -y nfs-kernel-server zip unzip python3-pkgconfig cython3 capnproto libcapnp-dev
elif [[ -x $(which yum) ]]
then
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/CentOS-Base.repo
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/epel.repo
    cat /etc/yum.repos.d/CentOS-Base.repo
    cat /etc/yum.conf
    grep -r abn-centosrepo /etc/yum.repos.d/* /etc/yum.conf || true

    yum install -y gcc gcc-c++ make capnproto-devel capnproto-libs capnproto nfs-utils zip python3-pkgconfig cython3
else
    echo "Can't find package management system"
    exit 1
fi
