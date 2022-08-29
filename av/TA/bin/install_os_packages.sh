#!/bin/bash

set -ex

if [[ -x $(which apt) ]]
then
    apt-get update
    # Retry 10 times before timeout
    for (( i=0; i<10; i++ ))
    do
       if DEBIAN_FRONTEND=noninteractive apt-get install -y nfs-kernel-server zip unzip samba p7zip-full gdb util-linux bfs ntfs-3g libguestfs-reiserfs
       then
          echo "Installation succeeded"
          break
       else
          echo "Failed to install packages retrying after sleep..."
          sleep 10
       fi
    done

elif [[ -x $(which yum) ]]
then
    if [[ -f /etc/os-release ]]; then
      . /etc/os-release
      OS=$NAME
    fi

    if [[ "$OS" == "CentOS Linux" ]]
    then
      for FILE in /etc/yum.repos.d/*
      do
        if [ -f "$FILE" ]; then
          if grep -q "abn-centosrepo/" "$FILE"; then
            sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' "FILE"
          fi
        fi
      done
    else
        echo "$OS is not CentOS"
    fi
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/CentOS-Base.repo
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/epel.repo

    if [[ -f /etc/yum.repos.d/CentOS-Base.repo ]]
    then
      cat /etc/yum.repos.d/CentOS-Base.repo
      cat /etc/yum.conf

      grep -r abn-centosrepo /etc/yum.repos.d/* /etc/yum.conf || true
    fi

    yum check-update || {
        EXIT=$?
        if (( EXIT == 100 ))
        then
            echo "Updates available!"
        else
            echo "yum check-update failed: $EXIT"
            exit $EXIT
        fi
    }
    yum install -y "gcc" "gcc-c++" "make" nfs-utils zip samba p7zip gdb util-linux ntfs-3g
else
    echo "Can't find package management system"
    exit 1
fi
echo "Completed install_os_packages.sh"
exit 0

