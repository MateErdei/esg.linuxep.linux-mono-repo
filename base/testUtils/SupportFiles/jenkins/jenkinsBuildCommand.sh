#!/usr/bin/env bash
set -x


JENKINS_DIR=$(dirname ${0})

## USAGE

#  <repo>_BRANCH=<desired_branch> <repo>_SOURCE=<build_path> jenkinsBuildCommand.sh  [Robot Arguments]
#
# example:
#   BASE_BRANCH=bugfix/LINUXDAR-999-pluginapi-fix MDR_PLUGIN_SOURCE=/uk-filer5/prodro/bir/sspl-mdr-control-plugin/1-0-0-45/217122/output/SDDS-COMPONENT/ jenkinsBuildCommand.sh -s mdr_plugin
#
#   this example would run the mdr_plugin suite with overrides for a dev branch of base and
#   using a production build of mdr plugin, with all else being the develop branch builds
#   from JenkinsBuildOutput on filer6
#
#  Overrides will also be picked up from environment variables set via "export" commands/etc.

# Valid <repo> values are:
#   BASE
#   EXAMPLE_PLUGIN
#   AUDIT_PLUGIN
#   EVENT_PROCESSOR
#   MDR_PLUGIN
#   EDR_PLUGIN
#   THININSTALLER
#   MDR_COMPONENT_SUITE

# Valid <build_path> values are usually to the SDDS_COMPONENT directory (if one exists)
# If in doubt, consult the default values under the "## SOURCE OVERRIDES" comment in this file
# Your directory should contain similar things to the default filer6 directory


date
#Sleep to give machines time to stabilise
sleep 10


function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}
PLATFORM_EXCLUDE_TAG=""
function platform_exclude_tag()
{
    if [[ -f /etc/centos-release ]]
    then
        current_release=$(cat /etc/centos-release)
        release_pattern="CentOS.*8.*"
        if [[ ${current_release} =~ ${release_pattern} ]]
        then
            PLATFORM_EXCLUDE_TAG="EXCLUDE_CENTOS8"
        else
            PLATFORM_EXCLUDE_TAG="EXCLUDE_CENTOS"
        fi
    elif [[ -f /etc/redhat-release ]]
    then
        current_release=$(cat /etc/redhat-release)
        release_pattern="Red Hat.*8.*"

        if [[ ${current_release} =~ ${release_pattern} ]]
        then
            PLATFORM_EXCLUDE_TAG="EXCLUDE_RHEL8"
        else
            PLATFORM_EXCLUDE_TAG="EXCLUDE_RHEL"
        fi
    elif [[ -n `lsb_release -a` ]]
    then
        current_release=$(cat /etc/os-release | grep PRETTY_NAME)
        release_pattern="PRETTY_NAME=\"Ubuntu 20.*"
        if [[ ${current_release} =~ ${release_pattern} ]]
        then
            PLATFORM_EXCLUDE_TAG="EXCLUDE_UBUNTU20"
        else
            PLATFORM_EXCLUDE_TAG="EXCLUDE_UBUNTU"
        fi

    fi
}

EXPECTED_WORKSPACE="/home/jenkins/workspace/Everest-SystemProductTests/label/(RhelCloneBuilder|Rhel8CloneBuilder|CentOSCloneBuilder|UbuntuCloneBuilder)"
if [[ $WORKSPACE =~ $EXPECTED_WORKSPACE ]]
then
    sudo cp $WORKSPACE/testUtils/SupportFiles/jenkins/auditdConfig.txt /etc/audit/auditd.conf || fail "ERROR: failed to copy auditdConfig from $WORKSPACE/SupportFiles to /etc/audit/auditd.conf"
fi

export TEST_UTILS=$WORKSPACE/testUtils
source $WORKSPACE/testUtils/SupportFiles/jenkins/gatherTestInputs.sh                || fail "Error: failed to gather test inputs"
source $WORKSPACE/testUtils/SupportFiles/jenkins/exportInputLocations.sh            || fail "Error: failed to export expected input locations"
source $WORKSPACE/testUtils/SupportFiles/jenkins/checkTestInputsAreAvailable.sh     || fail "Error: failed to validate gathered inputs"


bash ${JENKINS_DIR}/install_dependencies.sh

ROBOT_BASE_COMMAND="sudo -E python3 -m robot -x robot.xml --loglevel TRACE "
RERUNFAILED=${RERUNFAILED:-false}
HasFailure=false

inputArguments="$@"

platform_exclude_tag
EXCLUDED_BY_DEFAULT_TAGS="MANUAL PUB_SUB FUZZ AUDIT_PLUGIN EVENT_PLUGIN TESTFAILURE MCS_FUZZ AMAZON_LINUX EXAMPLE_PLUGIN "$PLATFORM_EXCLUDE_TAG

EXTRA_ARGUMENTS=""
for EXCLUDED_TAG in ${EXCLUDED_BY_DEFAULT_TAGS}; do
# if EXCLUDED_TAG is not mentioned in the input argument, add it to the default exclusion
if ! [[ $inputArguments =~ "${EXCLUDED_TAG}" ]]; then
  EXTRA_ARGUMENTS="${EXTRA_ARGUMENTS} -e ${EXCLUDED_TAG}"
fi
done

ROBOT_BASE_COMMAND="${ROBOT_BASE_COMMAND} ${EXTRA_ARGUMENTS}"

#Setup crash dumps for when a segfault occurs during system tests
sudo sysctl kernel.core_pattern=/tmp/core-%e-%s-%u-%g-%p-%t
ulimit -c unlimited

echo "Running command: ${ROBOT_BASE_COMMAND} $@"
eval $ROBOT_BASE_COMMAND $@ . || HasFailure=true

if [[ ${RERUNFAILED} == true && ${HasFailure} == true ]]; then
    echo "Running for re-run"
    sudo -E mv output.xml output1.xml
    sudo -E python3 -m robot --rerunfailed output1.xml --output output2.xml  . || echo "Failed tests on rerun"
    sudo -E rebot --merge --output output.xml -l log.html -r report.html output1.xml output2.xml
fi

if [[ $WORKSPACE =~ $EXPECTED_WORKSPACE ]]
then
    sudo chown -R jenkins:jenkins ${WORKSPACE} || fail "ERROR: failed to chown "$WORKSPACE
fi

sudo find /home/jenkins/jenkins_slave -name "*-cleanup_*" -exec rm -rf {} \; || exit 0
