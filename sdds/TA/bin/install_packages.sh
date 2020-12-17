#!/bin/bash

set -ex

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd "$STARTINGDIR"


TAP_PIP_INDEX_URL=$1

mkdir -p /opt/test/logs
mkdir -p /opt/test/results

echo "A" >/opt/test/logs/A
echo "B" >/opt/test/results/B

ls -lR /opt/test >/opt/test/logs/LS

if [[ -x $(which apt) ]]
then
    if [[ $(lsb_release -rs) == "18.04" ]]; then
        apt-get install -y python3.7 python3.7-dev
    else
        apt-get install -y python3.8 python3.8-dev
    fi
    apt-get install -y zip unzip
elif [[ -x $(which yum) ]]
then
    ping -c2 abn-centosrepo || true
    ping -c2 abn-engrepo.eng.sophos || true
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/CentOS-Base.repo
#    sed -i -e's/abn-centosrepo/abn-engrepo.eng.sophos/g' /etc/yum.repos.d/epel.repo
#    cat /etc/yum.repos.d/CentOS-Base.repo
#    cat /etc/yum.conf
    grep -r abn-centosrepo /etc/yum.repos.d/* /etc/yum.conf || true

    yum install -y "gcc" "gcc-c++" "make" "capnproto-devel" "capnproto-libs" "capnproto" zip
else
    echo "Can't find package management system"
    exit 1
fi


#    pip_index_args = ['--index-url', pip_index] if pip_index else []
#    pip_index_args += ["--no-cache-dir",
#                       "--progress-bar", "off",
#                       "--disable-pip-version-check",
#                       "--default-timeout", "120"]
#    machine.run(pip(machine), '--log', '/opt/test/logs/pip.log',
#                'install', '-v', *install_args, *pip_index_args,
#                log_mode=tap.LoggingMode.ON_ERROR)

REQUIREMENTS=$BASE/../requirements.txt

if [[ -f $REQUIREMENTS ]]
then
    if [[ -n $TAP_PIP_INDEX_URL ]]
    then
        TAP_PIP_INDEX_ARGS="--index-url $TAP_PIP_INDEX_URL"
    else
        TAP_PIP_INDEX_ARGS=
    fi

    pip3 \
        --log /opt/test/logs/pip.log install -v \
        -r $BASE/../requirements.txt \
        $TAP_PIP_INDEX_ARGS --no-cache-dir --progress-bar off --disable-pip-version-check \
        --default-timeout 120
else
    echo "No requirements found! $REQUIREMENTS" >&2
fi
