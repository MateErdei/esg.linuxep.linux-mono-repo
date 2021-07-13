#!/bin/bash

set -x

function failure()
{
    local CODE=$1
    shift
    echo "$@"
    exit $CODE
}

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR || exit 1
#PLATFORM_EXCLUDE_TAG=""
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

export TEST_UTILS=$SCRIPT_DIR


bash $SCRIPT_DIR/bin/install_os_packages.sh
bash $SCRIPT_DIR/bin/install_pip_requirements.sh
python3 -m pip install -r $SCRIPT_DIR/requirements.txt

echo "Running tests on $HOSTNAME"
RESULT=0
#EXCLUSIONS='-e MANUAL -e AUDIT_PLUGIN -e PUB_SUB -e EVENT_PLUGIN -e EXCLUDE_AWS -e CUSTOM_LOCATION -e TESTFAILURE -e FUZZ -e MCS_FUZZ -e MDR_REGRESSION_TESTS -e EXAMPLE_PLUGIN'
#python3 -m robot --include "INTEGRATION OR PRODUCT" --exclude "OSTIA OR MANUAL OR DISABLED OR STRESS" --removekeywords WUKS  .
python3 -m robot --include "AVSCANNER" --exclude "INTEGRATION OR OSTIA OR MANUAL OR DISABLED OR STRESS" --removekeywords WUKS  .

[[ ${RERUNFAILED} == true ]] || exit 0

if [[ ${RESULT} != 0 ]]; then
echo "Re-run failed tests"
    mv output.xml output1.xml
    python3 -m robot --rerunfailed output1.xml --output output2.xml  tests || echo "Failed tests on rerun"
    python3 -m robot.rebot --merge --output output.xml -l log.html -r report.html output1.xml output2.xml
fi

exit $RESULT