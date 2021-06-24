#!/bin/bash

set -ex

function wait_for_apt()
{
  while fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do
    sleep 1
  done
}

function apt_install()
{
    for x in 1 2 3 4 5
    do
        wait_for_apt
        apt-get install -y "$@" && return
    done
    echo "Failed to install $*"
    exit 1
}

if [[ -x $(which apt-get) ]]
then
    COMMON="nfs-kernel-server zip unzip python3-pkgconfig capnproto libcapnp-dev"
    if [[ $(lsb_release -rs) == "18.04" ]]; then
        apt_install python3.7 python3.7-dev $COMMON
    else
        apt_install python3.8 python3.8-dev $COMMON
    fi
elif [[ -x $(which yum) ]]
then
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/CentOS-Base.repo
    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/epel.repo
    cat /etc/yum.repos.d/CentOS-Base.repo
    cat /etc/yum.conf
    grep -r abn-centosrepo /etc/yum.repos.d/* /etc/yum.conf || true

    yum install -y gcc gcc-c++ make capnproto-devel capnproto-libs capnproto nfs-utils zip python3-pkgconfig
else
    echo "Can't find package management system"
    exit 1
fi
