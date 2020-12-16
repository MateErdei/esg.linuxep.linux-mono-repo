#!/bin/bash


set -ex

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd "$STARTINGDIR"

IDES=/tmp/ides
INSTALL_SET=/tmp/install_set
SOPHOS_INSTALL=/opt/sophos-spl
SCAN_TARGET=/tmp/basicRegression.tar.bz2
AVSCANNER=/usr/local/bin/avscanner

function record_memory_usage()
{
    ps -C sophos_threat_d --no-headers -o pid,comm,pcpu,cputime,pmem,rss,vsz,size
}

function add_ide()
{
    local IDE=$1

    cp $IDE $INSTALL_SET/files/plugins/av/chroot/susi/update_source/vdl/
    SOPHOS_INSTALL=$SOPHOS_INSTALL \
        bash $INSTALL_SET/install.sh
    sleep 5
}

function remove_ides()
{
    rm -f $INSTALL_SET/files/plugins/av/chroot/susi/update_source/vdl/*.ide
    SOPHOS_INSTALL=$SOPHOS_INSTALL \
        bash $INSTALL_SET/install.sh
    sleep 5
}

function scan_archive()
{
    "$AVSCANNER" -a "${SCAN_TARGET}" || echo "RESULT=$?"
}

LOG=/tmp/stress_log
echo "PID,COMM,PCPU,CPUTIME,PMEM,RSS,VSZ,SIZE" >$LOG

echo "Install if required:"
if [[ ! -d $SOPHOS_INSTALL ]]
then
    $BASE/installAV.sh
fi

for IDE in $IDES/*.ide
do
    add_ide "$IDE"
    scan_archive
    record_memory_usage >>$LOG
done

remove_ides
scan_archive
record_memory_usage >>$LOG


echo "All Done"
