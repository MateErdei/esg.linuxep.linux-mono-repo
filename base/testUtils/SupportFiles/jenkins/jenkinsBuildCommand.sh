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

[[ -z $SYSTEMPRODUCT_TEST_INPUT ]] && SYSTEMPRODUCT_TEST_INPUT=/tmp/system-product-test-inputs

SUDO="sudo "
[[ $(id -u) == 0 ]] && SUDO=

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
    ${SUDO}cp $WORKSPACE/base/testUtils/SupportFiles/jenkins/auditdConfig.txt /etc/audit/auditd.conf || fail "ERROR: failed to copy auditdConfig from $WORKSPACE/SupportFiles to /etc/audit/auditd.conf"
fi

export TEST_UTILS=$WORKSPACE/base/testUtils
rsync -auv $WORKSPACE/common/TA/libs/ ${TEST_UTILS}/libs/
cp -r $WORKSPACE/common/TA/robot ${TEST_UTILS}/
bash ${JENKINS_DIR}/install_dependencies.sh
bash ${JENKINS_DIR}/install_setup_tools.sh
[[ -n $NO_GATHER ]] || source $WORKSPACE/base/testUtils/SupportFiles/jenkins/gatherTestInputs.sh                || fail "Error: failed to gather test inputs"
source $WORKSPACE/base/testUtils/SupportFiles/jenkins/exportInputLocations.sh            || fail "Error: failed to export expected input locations"
source $WORKSPACE/base/testUtils/SupportFiles/jenkins/checkTestInputsAreAvailable.sh     || fail "Error: failed to validate gathered inputs"
python3 ${TEST_UTILS}/libs/DownloadAVSupplements.py || fail "Error: failed to gather av supplements locations"
#setup coverage inputs and exports
COVERAGE_STAGING="$SYSTEMPRODUCT_TEST_INPUT/coverage"


if [[ -n "${BASE_COVERAGE:-}" ]]; then
  # download tap + unit test cov file from Allegro, and use it to get combined (tap + unit + system tests)
  export FILESTODOWNLOAD=sspl-base-taptest/sspl-base-taptests.cov
  bash -x $WORKSPACE/base/build/bullseye/downloadFromAllegro.sh || fail "ERROR failed to download cov file, exit code:"$?
  mv /tmp/allegro/sspl-base-taptests.cov $COVERAGE_STAGING/sspl-base-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-base-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-base-combined
  export COV_HTML_BASE=sspl-base-combined
  export BULLSEYE_UPLOAD=1
  export UPLOAD_PATH=UnifiedPipelines/linuxep/everest-base
elif [[ -n "${EDR_COVERAGE:-}" ]]; then
  # download tap + unit test cov file from Allegro, and use it to get combined (tap + unit + system tests)
  export FILESTODOWNLOAD=sspl-plugin-edr-taptest/sspl-plugin-edr-tap.cov
  bash -x $WORKSPACE/base/build/bullseye/downloadFromAllegro.sh || fail "ERROR failed to download cov file, exit code:"$?
  mv /tmp/allegro/sspl-plugin-edr-tap.cov $COVERAGE_STAGING/sspl-plugin-edr-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-plugin-edr-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-plugin-edr-combined
  export COV_HTML_BASE=sspl-plugin-edr-combined
  export BULLSEYE_UPLOAD=1
  export UPLOAD_PATH=UnifiedPipelines/linuxep/sspl-plugin-edr-component
elif [[ -n "${PLUGIN_TEMPLATE_COVERAGE:-}" ]]; then
  # download tap + unit test cov file from Allegro, and use it to get combined (tap + unit + system tests)
  export FILESTODOWNLOAD=sspl-plugin-template-taptest/sspl-plugin-template-tap.cov
  bash -x $WORKSPACE/base/build/bullseye/downloadFromAllegro.sh || fail "ERROR failed to download cov file, exit code:"$?
  mv /tmp/allegro/sspl-plugin-template-tap.cov $COVERAGE_STAGING/sspl-plugin-template-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-plugin-template-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-plugin-template-combined
  export COV_HTML_BASE=sspl-plugin-template-combined
  export BULLSEYE_UPLOAD=1
  export UPLOAD_PATH=UnifiedPipelines/linuxep/sspl-plugin-template
elif [[ -n "${PLUGIN_EVENTJOURNALER_COVERAGE:-}" ]]; then
  # download tap + unit test cov file from Allegro, and use it to get combined (tap + unit + system tests)
  export FILESTODOWNLOAD=sspl-plugin-eventjournaler-taptest/sspl-plugin-eventjournaler-tap.cov
  bash -x $WORKSPACE/base/build/bullseye/downloadFromAllegro.sh || fail "ERROR failed to download cov file, exit code:"$?
  mv /tmp/allegro/sspl-plugin-eventjournaler-tap.cov $COVERAGE_STAGING/sspl-plugin-eventjournaler-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-plugin-eventjournaler-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-plugin-eventjournaler-combined
  export COV_HTML_BASE=sspl-plugin-eventjournaler-combined
  export BULLSEYE_UPLOAD=1
  export UPLOAD_PATH=UnifiedPipelines/linuxep/sspl-plugin-event-journaler
elif [[ -n "${LIVERESPONSE_COVERAGE:-}" ]]; then
  # download tap + unit test cov file from Allegro, and use it to get combined (tap + unit + system tests)
  export FILESTODOWNLOAD=sspl-plugin-liveterminal-taptest/sspl-plugin-liveterminal-tap.cov
  bash -x $WORKSPACE/base/build/bullseye/downloadFromAllegro.sh || fail "ERROR failed to download cov file, exit code:"$?
  mv /tmp/allegro/sspl-plugin-liveterminal-tap.cov $COVERAGE_STAGING/sspl-liveresponse-combined.cov
  # Begin merging the combined coverage with the unit-test coverage
  mv $COVERAGE_STAGING/covfile/liveterminal_unittests.cov $COVERAGE_STAGING/sspl-liveresponse-combined.cov
  export COVFILE=$COVERAGE_STAGING/sspl-liveresponse-combined.cov
  export htmldir=$COVERAGE_STAGING/sspl-liveresponse-combined
  export COV_HTML_BASE=sspl-liveresponse-combined
  export BULLSEYE_UPLOAD=
  export UPLOAD_PATH=UnifiedPipelines/winep/liveterminal
fi

SUDOE="sudo -E "
[[ $(id -u) == 0 ]] && SUDOE=

${SUDOE} python3 -m pip install --upgrade robotframework
ROBOT_BASE_COMMAND="${SUDOE}python3 -m robot -x ${WORKSPACE}/base/testUtils/robot.xml --loglevel TRACE "
RERUNFAILED=${RERUNFAILED:-false}
HasFailure=false

inputArguments="$*"

platform_exclude_tag
EXCLUDED_BY_DEFAULT_TAGS="MANUAL PUB_SUB FUZZ TESTFAILURE MCS_FUZZ AMAZON_LINUX EXAMPLE_PLUGIN "$PLATFORM_EXCLUDE_TAG

EXTRA_ARGUMENTS=""
for EXCLUDED_TAG in ${EXCLUDED_BY_DEFAULT_TAGS}; do
# if EXCLUDED_TAG is not mentioned in the input argument, add it to the default exclusion
if ! [[ $inputArguments =~ "${EXCLUDED_TAG}" ]]; then
  EXTRA_ARGUMENTS="${EXTRA_ARGUMENTS} -e ${EXCLUDED_TAG}"
fi
done

ROBOT_BASE_COMMAND="${ROBOT_BASE_COMMAND} ${EXTRA_ARGUMENTS}"

#Setup crash dumps for when a segfault occurs during system tests
${SUDO}sysctl kernel.core_pattern=/tmp/core-%e-%s-%u-%g-%p-%t
ulimit -c unlimited

echo "Running command: ${ROBOT_BASE_COMMAND} $@"
eval $ROBOT_BASE_COMMAND "$@" . || HasFailure=true

if [[ ${RERUNFAILED} == true && ${HasFailure} == true ]]; then
    echo "Running for re-run"
    ${SUDOE}mv output.xml output1.xml
    ${SUDOE}python3 -m robot --rerunfailed output1.xml --output output2.xml  . || echo "Failed tests on rerun"
    ${SUDOE}rebot --merge --output output.xml -l log.html -r report.html output1.xml output2.xml
fi

#upload coverage results
if [[ -n "${BASE_COVERAGE:-}" || -n "${EDR_COVERAGE:-}" || -n "${LIVERESPONSE_COVERAGE:-}" || -n "${PLUGIN_TEMPLATE_COVERAGE:-}" || -n "${PLUGIN_EVENTJOURNALER_COVERAGE:-}" ]]; then
  export COVERAGE_SCRIPT="$SYSTEMPRODUCT_TEST_INPUT/bazel-tools/tools/src/bullseye/test_coverage.py"
  bash -x $WORKSPACE/build/bullseye/uploadResults.sh || fail "ERROR failed to upload results exit code:"$?
fi


if [[ $WORKSPACE =~ $EXPECTED_WORKSPACE ]]
then
    ${SUDO}chown -R jenkins:jenkins ${WORKSPACE} || fail "ERROR: failed to chown "$WORKSPACE
fi

if [[ -d /home/jenkins/jenkins_slave ]]
then
    ${SUDO}find /home/jenkins/jenkins_slave -name "*-cleanup_*" -exec rm -rf {} \; || exit 0
fi
