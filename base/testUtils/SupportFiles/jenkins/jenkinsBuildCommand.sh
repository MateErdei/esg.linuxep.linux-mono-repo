#!/usr/bin/env bash
set -x


JENKINS_DIR=$(dirname ${0})

## USAGE

#  WORKSPACE=<path_to_base_repo_root>  jenkinsBuildCommand.sh  [Robot Arguments]
#  WORKSPACE is implicit in jenkins jobs
#  "s in robot args must be escaped
# example:
#   WORKSPACE=/vagrant/everest-base jenkinsBuildCommand.sh -t \"test name\"
#
#  build inputs are defined in testUtils/system-product-test-release-package.xml
#  the ostia VUT warehouse can also be set there by changing the args for the build step which calls ./setOstiaVutBranch.sh

#  AWS runner jobs which checkout this branch of base will inherit inputs from testUtils/system-product-test-release-package.xml

#  When running a custom jenkins job, testUtils/system-product-test-release-package.xml could be changed/overwritten
#  in place before running this script to change the inputs without pushing code. This will not trickle down
#  into any aws runner jobs.


date

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
[[ -n $NO_GATHER ]] || source $WORKSPACE/testUtils/SupportFiles/jenkins/gatherTestInputs.sh                || fail "Error: failed to gather test inputs"
source $WORKSPACE/testUtils/SupportFiles/jenkins/exportInputLocations.sh            || fail "Error: failed to export expected input locations"
source $WORKSPACE/testUtils/SupportFiles/jenkins/checkTestInputsAreAvailable.sh     || fail "Error: failed to validate gathered inputs"

#setup coverage inputs and exports
COVERAGE_STAGING=/tmp/system-product-test-inputs/coverage
if [[ -n "${BASE_COVERAGE:-}" ]]; then
  mv $COVERAGE_STAGING/sspl-base-unittest.cov $COVERAGE_STAGING/sspl-base-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-base-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-base-combined
  export COV_HTML_BASE=sspl-base-combined
  export BULLSEYE_UPLOAD=1
elif [[ -n "${MDR_COVERAGE:-}" ]]; then
  mv $COVERAGE_STAGING/sspl-mtr-unittest.cov $COVERAGE_STAGING/sspl-mtr-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-mtr-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-mtr-combined
  export COV_HTML_BASE=sspl-mtr-combined
  export BULLSEYE_UPLOAD=1
elif [[ -n "${LIVERESPONSE_COVERAGE:-}" ]]; then
  mv $COVERAGE_STAGING/liveterminal_unittests.cov $COVERAGE_STAGING/sspl-liveresponse-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-liveresponse-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-liveresponse-combined
  export COV_HTML_BASE=sspl-liveresponse-combined
  export BULLSEYE_UPLOAD=1
fi

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

#upload coverage results
if [[ -n "${BASE_COVERAGE:-}" || -n "${MDR_COVERAGE:-}" || -n "${LIVERESPONSE_COVERAGE:-}" ]]; then
  bash -x $WORKSPACE/build/bullseye/uploadResults.sh || fail "ERROR failed to upload results exit code:"$?
fi

if [[ $WORKSPACE =~ $EXPECTED_WORKSPACE ]]
then
    sudo chown -R jenkins:jenkins ${WORKSPACE} || fail "ERROR: failed to chown "$WORKSPACE
fi

sudo find /home/jenkins/jenkins_slave -name "*-cleanup_*" -exec rm -rf {} \; || exit 0
