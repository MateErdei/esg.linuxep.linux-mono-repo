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
        DEBIAN_FRONTEND=noninteractive apt-get install -y "$@" && return
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
# Change the ping check to check abn-centosrepo to abn-centosrepo.eng.sophos -> This will sanity check that you can actually reach the suggested update location

# Change all the sed commands to change abn-centosrepo to abn.centosrepo.eng.sophos for all files in /etc/yum.repos.d/, i.e. iterate through all files

# Try and make the above sed command only change abn-centosrepo to include the .eng.sophos if it requires it. Looks like
# the centos7 template has the correct path and the centos8 template doesn't so you want it to fix both. Perhaps gate
# the sed command being run on a grep of the files in the repo not containing abn-centosrepo.eng.sophos

    if [[ -f /etc/os-release ]]; then
      . /etc/os-release
      OS=$NAME
    fi

    if [[ "$OS" == "CentOS Linux" ]]
    then
      for FILE in /etc/yum.repos.d/*; do
        if [ -f "$FILE" ]; then
          if grep -q "abn-centosrepo/" "$FILE"; then
            sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' "$FILE"
          fi
        fi
      done
    fi
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
    cat /etc/yum.repos.d/CentOS-Base.repo || true
    cat /etc/yum.conf || true
    grep -r abn-centosrepo /etc/yum.repos.d/* /etc/yum.conf || true

    yum install -y gcc gcc-c++ make capnproto-devel capnproto-libs capnproto nfs-utils zip python3-pkgconfig
else
    echo "Can't find package management system"
    exit 1
fi
