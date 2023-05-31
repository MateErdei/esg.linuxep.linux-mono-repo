#!/bin/bash

set -ex

if [[ -x $(which apt) ]]
then
    PACKAGES="nfs-kernel-server zip unzip samba gdb util-linux bfs ntfs-3g libguestfs-reiserfs netcat"
    export DEBIAN_FRONTEND=noninteractive
    VERSION=$(sed -ne's/VERSION_ID="\(.*\)"/\1/p' /etc/os-release)
    case VERSION in
      18.04)
          TIMEOUT_UPDATE=
          TIMEOUT_INSTALL=
          ;;
      *)
          TIMEOUT_UPDATE="-o DPkg::Lock::Timeout=300"
          TIMEOUT_INSTALL="-o DPkg::Lock::Timeout=30"
          ;;
    esac
    apt $TIMEOUT_UPDATE update
    # Retry 10 times before timeout
    for (( i=0; i<10; i++ ))
    do
       if apt $TIMEOUT_INSTALL install -y $PACKAGES
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
    if [[ -f /etc/os-release ]]
    then
      . /etc/os-release
      OS=$NAME
    fi

    if [[ "$OS" == "CentOS Linux" ]]
    then
      for FILE in /etc/yum.repos.d/*
      do
        if [ -f "$FILE" ]; then
          if grep -q "abn-centosrepo/" "$FILE"; then
            sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' "$FILE"
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

    yum install -y "gcc" "gcc-c++" "make" nfs-utils zip samba gdb util-linux nc bzip2
    yum install -y ntfs-3g

elif [[ -x $(which zypper) ]]
then
    # Wait up to 10 minutes to get lock
    export ZYPP_LOCK_TIMEOUT=600
    # Retry 10 times before timeout
    for (( i=0; i<10; i++ ))
    do
        systemctl status cloud-final 2>&1 | tee /tmp/cloud-final.status
        if grep "active (exited)" /tmp/cloud-final.status
        then
            break
        else
            echo "sleeping for 4"
            sleep 4
        fi
    done

    for (( i=0; i<10; i++ ))
    do
        if zypper --non-interactive refresh </dev/null
        then
            break
        fi
        sleep $(( i * 2 ))
    done

    for (( i=0; i<10; i++ ))
    do
        if zypper --non-interactive install libcap-progs nfs-kernel-server zip unzip samba gdb util-linux netcat-openbsd </dev/null
        then
            break
        fi
        sleep $(( i * 2 ))
    done

    [[ -x $(which setcap) ]] || {
      echo "Failed to install setcap - AV can't run"
      exit 1
    }
else
    echo "Can't find package management system"
    exit 1
fi
echo "Completed install_os_packages.sh"
exit 0

