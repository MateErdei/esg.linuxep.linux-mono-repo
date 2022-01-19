#!/bin/bash

set -x

INCLUDE_TAG="${1:-INTEGRATION OR PRODUCT}"

function failure()
{
    local CODE=$1
    shift
    echo "$@"
    exit $CODE
}

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR || exit 1
PLATFORM_EXCLUDE_TAG=""
#
#if [[ -f /etc/centos-release ]]
#then
#    current_release=$(cat /etc/centos-release)
#    release_pattern="CentOS.*8.*"
#    if [[ ${current_release} =~ ${release_pattern} ]]
#    then
#        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_CENTOS8"
#    else
#        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_CENTOS"
#    fi
#elif [[ -f /etc/redhat-release ]]
#then
#    current_release=$(cat /etc/redhat-release)
#    release_pattern="Red Hat.*8.*"
#
#    if [[ ${current_release} =~ ${release_pattern} ]]
#    then
#        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_RHEL8"
#    else
#        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_RHEL"
#    fi
#elif [[ -n `lsb_release -a` ]]
#then
#    current_release=$(cat /etc/os-release | grep PRETTY_NAME)
#    release_pattern="PRETTY_NAME=\"Ubuntu 20.*"
#    if [[ ${current_release} =~ ${release_pattern} ]]
#    then
#        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_UBUNTU20"
#    else
#        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_UBUNTU"
#    fi
#elif [[ -f /etc/os-release ]]
#then
#    current_release=$(cat /etc/os-release | grep PRETTY_NAME)
#    release_pattern="PRETTY_NAME=\"Amazon Linux 2\""
#    if [[ ${current_release} =~ ${release_pattern} ]]
#    then
#        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_AMAZON_LINUX"
#    fi
#fi

SAMBA_EXCLUDE_TAG=
if [[ ! -f /etc/samba/smb.conf ]]
then
    echo "Excluding SMB as /etc/samba/smb.conf doesn't exist"
    SAMBA_EXCLUDE_TAG="-e SMB"
elif [[ ! -x $(which mount.cifs) ]]
then
    echo "Excluding SMB as mount.cifs doesn't exist"
    SAMBA_EXCLUDE_TAG="-e SMB"
fi

export TEST_UTILS=$SCRIPT_DIR


bash $SCRIPT_DIR/bin/install_os_packages.sh
bash $SCRIPT_DIR/bin/install_pip_requirements.sh
python3 -m pip install -r $SCRIPT_DIR/requirements.txt

echo "Running tests on $HOSTNAME"
RESULT=0
python3 -m robot --include "${INCLUDE_TAG}" \
    --exclude "OSTIA OR MANUAL OR DISABLED OR STRESS" \
    $PLATFORM_EXCLUDE_TAG \
    $SAMBA_EXCLUDE_TAG \
    --removekeywords WUKS  .
RESULT=$?

[[ ${RERUNFAILED} == true ]] || exit $RESULT

if (( RESULT != 0 ))
then
    echo "Re-run failed tests"
    mv output.xml output1.xml
    python3 -m robot --rerunfailed output1.xml --output output2.xml  tests || echo "Failed tests on rerun"
    python3 -m robot.rebot --merge --output output.xml -l log.html -r report.html output1.xml output2.xml
fi

exit $RESULT
