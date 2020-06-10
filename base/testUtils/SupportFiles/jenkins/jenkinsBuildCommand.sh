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
#   using a production build of mdr plugin, with all else being the master branch builds
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

export BASE_DIST="/tmp/distribution"
export EXAMPLEPLUGIN_SDDS="/tmp/Example-Plugin-SDDS-COMPONENT"
export SSPL_AUDIT_PLUGIN_SDDS="/tmp/Audit-Plugin-SDDS-COMPONENT"
export SSPL_PLUGIN_EVENTPROCESSOR_SDDS="/tmp/Event-Processor-Plugin-SDDS-COMPONENT"
export SSPL_EDR_PLUGIN_SDDS="/tmp/EDR-Plugin-SDDS-COMPONENT"
export SSPL_MDR_PLUGIN_SDDS="/tmp/MDR-Plugin-SDDS-COMPONENT"
export SYSTEM_PRODUCT_TEST_OUTPUT="/tmp/SystemProductTestOutput"
export THIN_INSTALLER_OVERRIDE="/tmp/ThinInstaller"
export SDDS_SSPL_DBOS_COMPONENT="/tmp/SDDS-SSPL-DBOS-COMPONENT"
export SDDS_SSPL_OSQUERY_COMPONENT="/tmp/SDDS-SSPL-OSQUERY-COMPONENT"
export SDDS_SSPL_MDR_COMPONENT="/tmp/SDDS-SSPL-MDR-COMPONENT"
export SDDS_SSPL_MDR_COMPONENT_SUITE="/tmp/SDDS-SSPL-MDR-COMPONENT-SUITE"
export SSPL_LIVERESPONSE_PLUGIN_SDDS="/tmp/liveresponse-plugin-SDDS-COMPONENT"
export WEBSOCKET_SERVER="/tmp/websocket_server"


## BRANCH OVERRIDES
# You can override the specific branch to use of any jenkins dev build by providing
# one of the bellow environment variable variables when this script is called
# If a <repo>_BRANCH variable is given, it will use that specific branch from the jenkins build output on filer6
# If none is given, master will be assumed

[[ ! -z ${BASE_BRANCH} ]]                 || BASE_BRANCH="master"
[[ ! -z ${EXAMPLE_PLUGIN_BRANCH} ]]       || EXAMPLE_PLUGIN_BRANCH="master"
[[ ! -z ${AUDIT_PLUGIN_BRANCH} ]]         || AUDIT_PLUGIN_BRANCH="master"
[[ ! -z ${EVENT_PROCESSOR_BRANCH} ]]      || EVENT_PROCESSOR_BRANCH="master"
[[ ! -z ${MDR_PLUGIN_BRANCH} ]]           || MDR_PLUGIN_BRANCH="master"
[[ ! -z ${EDR_PLUGIN_BRANCH} ]]           || EDR_PLUGIN_BRANCH="master"
[[ ! -z ${LIVERESPONSE_PLUGIN_BRANCH} ]]  || LIVERESPONSE_PLUGIN_BRANCH="develop"
[[ ! -z ${THININSTALLER_BRANCH} ]]        || THININSTALLER_BRANCH="master"
[[ ! -z ${MDR_COMPONENT_SUITE_BRANCH} ]]  || MDR_COMPONENT_SUITE_BRANCH="master"

## SOURCE OVERRIDES
# If a <repo>_SOURCE variable is given, it will use the exact path you give it
# If none is given, the filer6 CI build output will be used (with the branch as defined above)
# Giving both a branch and source override renders the <repo>_BRANCH variable redundant

DEVBFR="/mnt/filer6/bfr"
LASTGOODBUILD () {
        echo "$1/$( cat "$1/lastgoodbuild.txt" )"
}
[[ ! -z  ${BASE_SOURCE} ]]                         || BASE_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-base/${BASE_BRANCH}" )/sspl-base/*/output/SDDS-COMPONENT)
[[ ! -z  ${EXAMPLE_PLUGIN_SOURCE} ]]               || EXAMPLE_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-exampleplugin/${EXAMPLE_PLUGIN_BRANCH}" )/sspl-exampleplugin/*/output/SDDS-COMPONENT)
[[ ! -z  ${AUDIT_PLUGIN_SOURCE} ]]                 || AUDIT_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-audit/${AUDIT_PLUGIN_BRANCH}" )/sspl-audit/*/output/SDDS-COMPONENT)
[[ ! -z  ${EVENT_PROCESSOR_SOURCE} ]]              || EVENT_PROCESSOR_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-eventprocessor/${EVENT_PROCESSOR_BRANCH}" )/sspl-eventprocessor/*/output/SDDS-COMPONENT)
[[ ! -z  ${MDR_PLUGIN_SOURCE} ]]                   || MDR_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-mdr-control-plugin/${MDR_PLUGIN_BRANCH}" )/sspl-mdr-control-plugin/*/output/SDDS-COMPONENT)
[[ ! -z  ${EDR_PLUGIN_SOURCE} ]]                   || EDR_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-plugin-edr-component/${EDR_PLUGIN_BRANCH}" )/sspl-plugin-edr-component/*/output/SDDS-COMPONENT)
[[ ! -z  ${THININSTALLER_SOURCE} ]]                || THININSTALLER_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-thininstaller/${THININSTALLER_BRANCH}" )/sspl-thininstaller/*/output)
[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE} ]]          || MDR_COMPONENT_SUITE_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-mdr-componentsuite/${MDR_COMPONENT_SUITE_BRANCH}" )/sspl-mdr-componentsuite/*/output)
# It is not expected that you will ever override the following locations as they are implied by previously set variables
[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_DBOS} ]]     || MDR_COMPONENT_SUITE_SOURCE_DBOS="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-DBOS-COMPONENT/"
[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_OSQUERY} ]]  || MDR_COMPONENT_SUITE_SOURCE_OSQUERY="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-OSQUERY-COMPONENT/"
[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_PLUGIN} ]]   || MDR_COMPONENT_SUITE_SOURCE_PLUGIN="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-MDR-COMPONENT/"
[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_SUITE} ]]    || MDR_COMPONENT_SUITE_SOURCE_SUITE="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-MDR-COMPONENT-SUITE/"
# The System Product Test Output must be from the build of base being used. It is found in the directory above the SDDS-COMPONENT
[[ ! -z  ${SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE} ]]   || SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE="$(dirname ${BASE_SOURCE})/SystemProductTestOutput.tar.gz"
[[ ! -z  ${LIVERESPONSE_PLUGIN_SOURCE} ]]          || LIVERESPONSE_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/liveterminal/${LIVERESPONSE_PLUGIN_BRANCH}" )/liveterminal_linux11/*/output/SDDS-COMPONENT)
[[ ! -z  ${WEBSOCKET_SERVER_SOURCE} ]]             || WEBSOCKET_SERVER_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/liveterminal/${LIVERESPONSE_PLUGIN_BRANCH}" )/liveterminal/*/websocket_server)

# Check everything exists

[[ -d ${BASE_SOURCE} ]]                         || fail "Error: ${BASE_SOURCE} doesn't exist"
[[ -d ${EXAMPLE_PLUGIN_SOURCE} ]]               || fail "Error: ${EXAMPLE_PLUGIN_SOURCE} doesn't exist"
[[ -d ${AUDIT_PLUGIN_SOURCE} ]]                 || fail "Error: ${AUDIT_PLUGIN_SOURCE} doesn't exist"
[[ -d ${EVENT_PROCESSOR_SOURCE} ]]              || fail "Error: ${EVENT_PROCESSOR_SOURCE} doesn't exist"
[[ -d ${MDR_PLUGIN_SOURCE} ]]                   || fail "Error: ${MDR_PLUGIN_SOURCE} doesn't exist"
[[ -d ${EDR_PLUGIN_SOURCE} ]]                   || fail "Error: ${EDR_PLUGIN_SOURCE} doesn't exist"
[[ -f ${SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE} ]]   || fail "Error: ${SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE} doesn't exist"
[[ -d ${THININSTALLER_SOURCE} ]]                || fail "Error: ${THININSTALLER_SOURCE} doesn't exist"
[[ -d ${MDR_COMPONENT_SUITE_SOURCE_DBOS} ]]     || fail "Error: ${MDR_COMPONENT_SUITE_SOURCE_DBOS} doesn't exist"
[[ -d ${MDR_COMPONENT_SUITE_SOURCE_OSQUERY} ]]  || fail "Error: ${MDR_COMPONENT_SUITE_SOURCE_OSQUERY} doesn't exist"
[[ -d ${MDR_COMPONENT_SUITE_SOURCE_PLUGIN} ]]   || fail "Error: ${MDR_COMPONENT_SUITE_SOURCE_PLUGIN} doesn't exist"
[[ -d ${MDR_COMPONENT_SUITE_SOURCE_SUITE} ]]    || fail "Error: ${MDR_COMPONENT_SUITE_SOURCE_SUITE} doesn't exist"
[[ -d ${SSPL_LIVERESPONSE_PLUGIN_SOURCE} ]]          || fail "Error: ${LIVERESPONSE_PLUGIN_SOURCE} doesn't exist"
[[ -d ${WEBSOCKET_SERVER_SOURCE} ]]             || fail "Error: ${WEBSOCKET_SERVER_SOURCE} doesn't exist"

sudo rm -rf ${BASE_DIST}
sudo rm -rf ${EXAMPLEPLUGIN_SDDS}
sudo rm -rf ${SSPL_AUDIT_PLUGIN_SDDS}
sudo rm -rf ${SSPL_PLUGIN_EVENTPROCESSOR_SDDS}
sudo rm -rf ${SSPL_MDR_PLUGIN_SDDS}
sudo rm -rf ${SSPL_EDR_PLUGIN_SDDS}
sudo rm -rf ${SYSTEM_PRODUCT_TEST_OUTPUT}
sudo rm -rf ${THIN_INSTALLER_OVERRIDE}
sudo rm -rf ${SDDS_SSPL_DBOS_COMPONENT}
sudo rm -rf ${SDDS_SSPL_OSQUERY_COMPONENT}
sudo rm -rf ${SDDS_SSPL_MDR_COMPONENT}
sudo rm -rf ${SDDS_SSPL_MDR_COMPONENT_SUITE}
sudo rm -rf ${SSPL_LIVERESPONSE_PLUGIN_SDDS}
sudo rm -rf ${WEBSOCKET_SERVER}

# make the Product Test Output folder so that the tar can be copied into it
mkdir ${SYSTEM_PRODUCT_TEST_OUTPUT}

sudo cp -r ${BASE_SOURCE} ${BASE_DIST}
sudo cp -r ${EXAMPLE_PLUGIN_SOURCE} ${EXAMPLEPLUGIN_SDDS}
sudo cp -r ${AUDIT_PLUGIN_SOURCE} ${SSPL_AUDIT_PLUGIN_SDDS}
sudo cp -r ${EVENT_PROCESSOR_SOURCE} ${SSPL_PLUGIN_EVENTPROCESSOR_SDDS}
sudo cp -r ${MDR_PLUGIN_SOURCE} ${SSPL_MDR_PLUGIN_SDDS}
sudo cp -r ${EDR_PLUGIN_SOURCE} ${SSPL_EDR_PLUGIN_SDDS}
sudo cp -r ${SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE} ${SYSTEM_PRODUCT_TEST_OUTPUT}
sudo cp -r ${THININSTALLER_SOURCE} ${THIN_INSTALLER_OVERRIDE}
sudo cp -r ${MDR_COMPONENT_SUITE_SOURCE_DBOS} ${SDDS_SSPL_DBOS_COMPONENT}
sudo cp -r ${MDR_COMPONENT_SUITE_SOURCE_OSQUERY} ${SDDS_SSPL_OSQUERY_COMPONENT}
sudo cp -r ${MDR_COMPONENT_SUITE_SOURCE_PLUGIN} ${SDDS_SSPL_MDR_COMPONENT}
sudo cp -r ${MDR_COMPONENT_SUITE_SOURCE_SUITE} ${SDDS_SSPL_MDR_COMPONENT_SUITE}
sudo cp -r ${LIVERESPONSE_PLUGIN_SOURCE} ${SSPL_LIVERESPONSE_PLUGIN_SDDS}
sudo cp -r ${WEBSOCKET_SERVER_SOURCE} ${WEBSOCKET_SERVER}

echo "Using ${BASE_SOURCE} as BASE_SOURCE"
echo "Using ${EXAMPLE_PLUGIN_SOURCE} as EXAMPLE_PLUGIN_SOURCE"
echo "Using ${AUDIT_PLUGIN_SOURCE} as AUDIT_PLUGIN_SOURCE"
echo "Using ${EVENT_PROCESSOR_SOURCE} as EVENT_PROCESSOR_SOURCE"
echo "Using ${MDR_PLUGIN_SOURCE} as MDR_PLUGIN_SOURCE"
echo "Using ${EDR_PLUGIN_SOURCE} as EDR_PLUGIN_SOURCE"
echo "Using ${SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE} as SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE"
echo "Using ${THININSTALLER_SOURCE} as THININSTALLER_SOURCE"
echo "Using ${MDR_COMPONENT_SUITE_SOURCE_DBOS} as MDR_COMPONENT_SUITE_DBOS_SDDS"
echo "Using ${MDR_COMPONENT_SUITE_SOURCE_OSQUERY} as MDR_COMPONENT_SUITE_OSQUERY_SDDS"
echo "Using ${MDR_COMPONENT_SUITE_SOURCE_PLUGIN} as MDR_COMPONENT_SUITE_PLUGIN_SDDS"
echo "Using ${MDR_COMPONENT_SUITE_SOURCE_SUITE} as MDR_COMPONENT_SUITE_SDDS"
echo "Using ${LIVERESPONSE_PLUGIN_SOURCE} as LIVERESPONSE_PLUGIN_SOURCE"
echo "Using ${WEBSOCKET_SERVER_SOURCE} as WEBSOCKET_SERVER_SOURCE"

bash ${JENKINS_DIR}/install_dependencies.sh

ROBOT_BASE_COMMAND="sudo -E python3 -m robot -x robot.xml --loglevel TRACE "
RERUNFAILED=${RERUNFAILED:-false}
HasFailure=false

inputArguments="$@"

platform_exclude_tag
EXCLUDED_BY_DEFAULT_TAGS="MANUAL PUB_SUB FUZZ AUDIT_PLUGIN EVENT_PLUGIN TESTFAILURE MCS_FUZZ CUSTOM_LOCATION AMAZON_LINUX EXAMPLE_PLUGIN "$PLATFORM_EXCLUDE_TAG

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
    sudo -E python3 -m robot --rerunfailed output1.xml --output output2.xml  .
    sudo -E rebot --merge --output output.xml -l log.html -r report.html output1.xml output2.xml
fi

if [[ $WORKSPACE =~ $EXPECTED_WORKSPACE ]]
then
    sudo chown -R jenkins:jenkins ${WORKSPACE} || fail "ERROR: failed to chown "$WORKSPACE
fi

#Copy segfault core dumps onto filer 6 at the end of test runs
if [[ `ls -al /tmp | grep core- | wc -l` != 0 ]]
then
    CRASH_DUMP_FILER6=/mnt/filer6/linux/SSPL/CoreDumps/${NODE_NAME}_${BUILD_TAG}
    sudo mkdir -p ${CRASH_DUMP_FILER6}
    sudo cp /tmp/core-* ${CRASH_DUMP_FILER6}
fi

sudo find /home/jenkins/jenkins_slave -name "*-cleanup_*" -exec rm -rf {} \; || exit 0
