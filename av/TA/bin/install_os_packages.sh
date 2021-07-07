#!/bin/bash

set -ex

if [[ -x $(which apt) ]]
then
    apt-get update
    apt install -y nfs-kernel-server zip unzip samba
elif [[ -x $(which yum) ]]
then
    if [[ -f /etc/os-release ]]; then
      . /etc/os-release
      OS=$NAME
    fi

    if [[ "$OS" -eq "CentOS" ]]; then
      for FILE in /etc/yum.repos.d/*; do
        if [ -f "$FILE" ]; then
          if grep -q "abn-centosrepo/" "$FILE"; then
            sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' "FILE"
          fi
        fi
      done
    fi
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/CentOS-Base.repo
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/epel.repo
    cat /etc/yum.repos.d/CentOS-Base.repo
    cat /etc/yum.conf
    grep -r abn-centosrepo /etc/yum.repos.d/* /etc/yum.conf || true

    yum check-update
    yum install -y "gcc" "gcc-c++" "make" nfs-utils zip samba
else
    echo "Can't find package management system"
    exit 1
fi
