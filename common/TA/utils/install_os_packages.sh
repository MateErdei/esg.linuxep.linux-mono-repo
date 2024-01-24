#!/bin/bash

set -ex

echo "install_os_packages.sh $*"

if [[ -x $(which apt) ]]
then
    export DEBIAN_FRONTEND=noninteractive
    TIMEOUT_UPDATE="-o DPkg::Lock::Timeout=300"
    TIMEOUT_INSTALL="-o DPkg::Lock::Timeout=30"

    for (( i=0; i<5; i++ ))
    do
       if apt $TIMEOUT_UPDATE update
       then
          break
       fi
       echo "Failed to apt update"
       sleep 5
    done

    # Retry 10 times before timeout
    for (( i=0; i<10; i++ ))
    do
       if apt $TIMEOUT_INSTALL install -y $*
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

    yum install -y $*
elif [[ -x $(which zypper) ]]
then
    # Wait up to 10 minutes to get lock
    export ZYPP_LOCK_TIMEOUT=600
    # Allow services to get established
    sleep 45s
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

    # Check registration
    SUSEConnect --status-text
    echo LOG cloudregister:
    tail -n 5 /var/log/cloudregister
    echo LOG cloud-init:
    tail -n 5 /var/log/cloud-init.log

    # Manage failed registration with retry
    for (( i=0; i<5; i++ ))
    do
        if grep -i "ERROR:	Registration failed:" /var/log/cloudregister
        then
            if grep -i "INFO:No current registration server set" /var/log/cloudregister
            then
                # Deregister
                echo LOG Deregister from SUSE
                registercloudguest --clean
                sleep 10s
                mv /var/log/cloudregister /var/log/cloudregister.FAIL
                mv /var/log/cloud-init.log /var/log/cloud-init.FAIL

                # Register again
                echo LOG Reregister with SUSE
                registercloudguest --force-new
                # Allow services to get established
                sleep 60s

                # Check registration
                SUSEConnect --status-text
                echo LOG cloudregister:
                tail -n 5 /var/log/cloudregister
            fi
        else
            break
        fi
    done

    zypper refs </dev/null

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
        if zypper --non-interactive install $* </dev/null
        then
            break
        fi
        sleep $(( i * 2 ))
    done
else
    echo "Can't find package management system"
    exit 1
fi
echo "Completed install_os_packages.sh"
exit 0

