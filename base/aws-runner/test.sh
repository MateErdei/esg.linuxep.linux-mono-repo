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
PLATFORM_EXCLUDE_TAG=""
PYTHONCMD="python3"

if [[ -f /etc/centos-release ]]
then
    current_release=$(cat /etc/centos-release)
    PLATFORM_EXCLUDE_TAG="-e EXCLUDE_CENTOS"
elif [[ -f /etc/oracle-release ]]
then
    current_release=$(cat /etc/oracle-release)
    release_patternOracle7="Oracle Linux Server release 7.*"
    release_patternOracle8="Oracle Linux Server release 8.*"
    if [[ ${current_release} =~ ${release_patternOracle7} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_ORACLE7"
    elif [[ ${current_release} =~ ${release_patternOracle8} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_ORACLE8"
    fi
elif [[ -f /etc/redhat-release ]]
then
    current_release=$(cat /etc/redhat-release)
    # Space added in front of major version to prevent it matching minor version
    release_patternRhel7="Red Hat.* 7.*"
    release_patternRhel8="Red Hat.* 8.*"
    release_patternRhel9="Red Hat.* 9.*"

    if [[ ${current_release} =~ ${release_patternRhel7} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_RHEL7"
    elif [[ ${current_release} =~ ${release_patternRhel8} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_RHEL8"
    elif [[ ${current_release} =~ ${release_patternRhel9} ]]
    then
        export OPENSSL_ENABLE_SHA1_SIGNATURES=1
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_RHEL9"
    else
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_RHEL"
    fi
elif [[ -n `lsb_release -a` ]]
then
    current_release=$(cat /etc/os-release | grep PRETTY_NAME)
    release_patternUbuntu22="PRETTY_NAME=\"Ubuntu 22.*"
    release_patternUbuntu20="PRETTY_NAME=\"Ubuntu 20.*"
    release_patternUbuntu18="PRETTY_NAME=\"Ubuntu 18.*"
    release_patternDebian10="PRETTY_NAME=\"Debian GNU/Linux 10.*"
    release_patternDebian11="PRETTY_NAME=\"Debian GNU/Linux 11.*"
    release_patternDebian12="PRETTY_NAME=\"Debian GNU/Linux 12.*"
    if [[ ${current_release} =~ ${release_patternUbuntu22} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_UBUNTU22"
    elif [[ ${current_release} =~ ${release_patternUbuntu20} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_UBUNTU20"
    elif [[ ${current_release} =~ ${release_patternUbuntu18} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_UBUNTU18"
    elif [[ ${current_release} =~ ${release_patternDebian10} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_DEBIAN10"
    elif [[ ${current_release} =~ ${release_patternDebian11} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_DEBIAN11"
    elif [[ ${current_release} =~ ${release_patternDebian12} ]]
    then
        PYTHONCMD="python3.9"
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_DEBIAN12"
    else
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_UBUNTU"
    fi
elif [[ -f /etc/os-release ]]
then
    current_release=$(cat /etc/os-release | grep PRETTY_NAME)
    release_patternAL2="PRETTY_NAME=\"Amazon Linux 2\""
    release_patternAL2023="PRETTY_NAME=\"Amazon Linux 2023\""
    # Glob required due to some SLES platforms having SPs, e.g. SUSE Linux Enterprise Server 15 SP4
    release_patternSLES12="PRETTY_NAME=\"SUSE Linux Enterprise Server 12.*\""
    release_patternSLES15="PRETTY_NAME=\"SUSE Linux Enterprise Server 15.*\""
    if [[ ${current_release} =~ ${release_patternAL2} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_AMAZON_LINUX2"
    elif [[ ${current_release} =~ ${release_patternAL2023} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_AMAZON_LINUX2023"
    elif [[ ${current_release} =~ ${release_patternSLES12} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_SLES12"
    elif [[ ${current_release} =~ ${release_patternSLES15} ]]
    then
        PLATFORM_EXCLUDE_TAG="-e EXCLUDE_SLES15"
    fi
fi

export TEST_UTILS=$SCRIPT_DIR
source $SCRIPT_DIR/SupportFiles/jenkins/exportInputLocations.sh || failure 21 "failed to export input locations"
# only unpack if location doesn't exist to prevent repeated unpacks during manual testing
[[ -d $SYSTEMPRODUCT_TEST_INPUT ]] ||  tar xzf $SCRIPT_DIR/SystemProductTestInputs.tgz -C $(dirname $SYSTEMPRODUCT_TEST_INPUT)
source $SCRIPT_DIR/SupportFiles/jenkins/checkTestInputsAreAvailable.sh || failure 22 "failed to validate input locations"

bash $SCRIPT_DIR/SupportFiles/jenkins/install_dependencies.sh

${PYTHONCMD} -m pip install -i https://pypi.org/simple -r requirements.txt

echo "Running tests on $HOSTNAME"
RESULT=0
EXCLUSIONS='-e MANUAL -e PUB_SUB -e EXCLUDE_AWS -e CUSTOM_LOCATION -e TESTFAILURE -e FUZZ -e MCS_FUZZ -e EXAMPLE_PLUGIN'
${PYTHONCMD}  -m robot -x robot.xml --loglevel TRACE ${EXCLUSIONS} ${PLATFORM_EXCLUDE_TAG} "$@" tests || RESULT=$?

[[ ${RERUNFAILED} == true ]] || exit 0

if [[ ${RESULT} != 0 ]]; then
echo "Re-run failed tests"
    mv output.xml output1.xml
    ${PYTHONCMD} -m robot --rerunfailed output1.xml --output output2.xml  tests || echo "Failed tests on rerun"
    ${PYTHONCMD} -m robot.rebot --merge --output output.xml -l log.html -r report.html output1.xml output2.xml
fi

exit $RESULT