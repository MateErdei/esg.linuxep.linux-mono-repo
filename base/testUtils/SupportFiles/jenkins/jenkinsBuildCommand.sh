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

SYSTEMPRODUCT_TEST_INPUT=/tmp/system-product-test-inputs
export BASE_DIST="$SYSTEMPRODUCT_TEST_INPUT/sspl-base"
export EXAMPLEPLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-exampleplugin"
export SSPL_AUDIT_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-audit"
export SSPL_PLUGIN_EVENTPROCESSOR_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-eventprocessor"
export SSPL_EDR_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-edr-plugin"
export SSPL_MDR_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-mdr-control-plugin"
export SYSTEM_PRODUCT_TEST_OUTPUT="$SYSTEMPRODUCT_TEST_INPUT/SystemProductTestOutput"
export SYSTEM_PRODUCT_TEST_OUTPUT_FILE="$SYSTEM_PRODUCT_TEST_OUTPUT/SystemProductTestOutput.tar.gz"
export THIN_INSTALLER_OVERRIDE="$SYSTEMPRODUCT_TEST_INPUT/sspl-thininstaller"
export SDDS_SSPL_DBOS_COMPONENT="$SYSTEMPRODUCT_TEST_INPUT/sspl-mdr-componentsuite/SDDS-SSPL-DBOS-COMPONENT"
export SDDS_SSPL_OSQUERY_COMPONENT="$SYSTEMPRODUCT_TEST_INPUT/sspl-mdr-componentsuite/SDDS-SSPL-OSQUERY-COMPONENT"
export SDDS_SSPL_MDR_COMPONENT="$SYSTEMPRODUCT_TEST_INPUT/sspl-mdr-componentsuite/SDDS-SSPL-MDR-COMPONENT"
export SDDS_SSPL_MDR_COMPONENT_SUITE="$SYSTEMPRODUCT_TEST_INPUT/sspl-mdr-componentsuite/SDDS-SSPL-MDR-COMPONENT-SUITE"
export SSPL_LIVERESPONSE_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/liveterminal"
export WEBSOCKET_SERVER="$SYSTEMPRODUCT_TEST_INPUT/websocket_server"

#
### BRANCH OVERRIDES
## You can override the specific branch to use of any jenkins dev build by providing
## one of the bellow environment variable variables when this script is called
## If a <repo>_BRANCH variable is given, it will use that specific branch from the jenkins build output on filer6
## If none is given, master will be assumed
#
#[[ ! -z ${BASE_BRANCH} ]]                 || BASE_BRANCH="master"
#[[ ! -z ${EXAMPLE_PLUGIN_BRANCH} ]]       || EXAMPLE_PLUGIN_BRANCH="master"
#[[ ! -z ${AUDIT_PLUGIN_BRANCH} ]]         || AUDIT_PLUGIN_BRANCH="master"
#[[ ! -z ${EVENT_PROCESSOR_BRANCH} ]]      || EVENT_PROCESSOR_BRANCH="master"
#[[ ! -z ${MDR_PLUGIN_BRANCH} ]]           || MDR_PLUGIN_BRANCH="master"
#[[ ! -z ${EDR_PLUGIN_BRANCH} ]]           || EDR_PLUGIN_BRANCH="master"
#[[ ! -z ${THININSTALLER_BRANCH} ]]        || THININSTALLER_BRANCH="master"
#[[ ! -z ${MDR_COMPONENT_SUITE_BRANCH} ]]  || MDR_COMPONENT_SUITE_BRANCH="master"
#
### SOURCE OVERRIDES
## If a <repo>_SOURCE variable is given, it will use the exact path you give it
## If none is given, the filer6 CI build output will be used (with the branch as defined above)
## Giving both a branch and source override renders the <repo>_BRANCH variable redundant
#
#DEVBFR="/mnt/filer6/bfr"
#LASTGOODBUILD () {
#        echo "$1/$( cat "$1/lastgoodbuild.txt" )"
#}
#[[ ! -z  ${BASE_SOURCE} ]]                         || BASE_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-base/${BASE_BRANCH}" )/sspl-base/*/output/SDDS-COMPONENT)
#[[ ! -z  ${EXAMPLE_PLUGIN_SOURCE} ]]               || EXAMPLE_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-exampleplugin/${EXAMPLE_PLUGIN_BRANCH}" )/sspl-exampleplugin/*/output/SDDS-COMPONENT)
#[[ ! -z  ${AUDIT_PLUGIN_SOURCE} ]]                 || AUDIT_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-audit/${AUDIT_PLUGIN_BRANCH}" )/sspl-audit/*/output/SDDS-COMPONENT)
#[[ ! -z  ${EVENT_PROCESSOR_SOURCE} ]]              || EVENT_PROCESSOR_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-eventprocessor/${EVENT_PROCESSOR_BRANCH}" )/sspl-eventprocessor/*/output/SDDS-COMPONENT)
#[[ ! -z  ${MDR_PLUGIN_SOURCE} ]]                   || MDR_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-mdr-control-plugin/${MDR_PLUGIN_BRANCH}" )/sspl-mdr-control-plugin/*/output/SDDS-COMPONENT)
#[[ ! -z  ${EDR_PLUGIN_SOURCE} ]]                   || EDR_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-plugin-edr-component/${EDR_PLUGIN_BRANCH}" )/sspl-plugin-edr-component/*/output/SDDS-COMPONENT)
#[[ ! -z  ${THININSTALLER_SOURCE} ]]                || THININSTALLER_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-thininstaller/${THININSTALLER_BRANCH}" )/sspl-thininstaller/*/output)
#[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE} ]]          || MDR_COMPONENT_SUITE_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-mdr-componentsuite/${MDR_COMPONENT_SUITE_BRANCH}" )/sspl-mdr-componentsuite/*/output)
## It is not expected that you will ever override the following locations as they are implied by previously set variables
#[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_DBOS} ]]     || MDR_COMPONENT_SUITE_SOURCE_DBOS="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-DBOS-COMPONENT/"
#[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_OSQUERY} ]]  || MDR_COMPONENT_SUITE_SOURCE_OSQUERY="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-OSQUERY-COMPONENT/"
#[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_PLUGIN} ]]   || MDR_COMPONENT_SUITE_SOURCE_PLUGIN="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-MDR-COMPONENT/"
#[[ ! -z  ${MDR_COMPONENT_SUITE_SOURCE_SUITE} ]]    || MDR_COMPONENT_SUITE_SOURCE_SUITE="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-MDR-COMPONENT-SUITE/"
## The System Product Test Output must be from the build of base being used. It is found in the directory above the SDDS-COMPONENT
#[[ ! -z  ${SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE} ]]   || SYSTEM_PRODUCT_TEST_OUTPUT_SOURCE="$(dirname ${BASE_SOURCE})/SystemProductTestOutput.tar.gz"
#
#
## Check everything exists
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

sudo $WORKSPACE/testUtils/SupportFiles/jenkins/SetupCIBuildScripts.sh || fail "Error: Failed to get CI scripts"
pushd $WORKSPACE
python3 -m build_scripts.artisan_fetch $WORKSPACE/testUtils/release-package.xml || fail "Error: Failed to fetch inputs"
popd

[[ -d ${BASE_DIST} ]]                           || fail "Error: ${BASE_DIST} doesn't exist"
[[ -d ${EXAMPLEPLUGIN_SDDS} ]]                  || fail "Error: ${EXAMPLEPLUGIN_SDDS} doesn't exist"
[[ -d ${SSPL_AUDIT_PLUGIN_SDDS} ]]              || fail "Error: ${SSPL_AUDIT_PLUGIN_SDDS} doesn't exist"
[[ -d ${SSPL_PLUGIN_EVENTPROCESSOR_SDDS} ]]     || fail "Error: ${SSPL_PLUGIN_EVENTPROCESSOR_SDDS} doesn't exist"
[[ -d ${SSPL_MDR_PLUGIN_SDDS} ]]                || fail "Error: ${SSPL_MDR_PLUGIN_SDDS} doesn't exist"
[[ -d ${SSPL_EDR_PLUGIN_SDDS} ]]                || fail "Error: ${SSPL_EDR_PLUGIN_SDDS} doesn't exist"
[[ -f ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} ]]     || fail "Error: ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} doesn't exist"
[[ -d ${SYSTEM_PRODUCT_TEST_OUTPUT} ]]          || fail "Error: ${SYSTEM_PRODUCT_TEST_OUTPUT} doesn't exist"
[[ -d ${THIN_INSTALLER_OVERRIDE} ]]             || fail "Error: ${THIN_INSTALLER_OVERRIDE} doesn't exist"
[[ -d ${SDDS_SSPL_DBOS_COMPONENT} ]]            || fail "Error: ${SDDS_SSPL_DBOS_COMPONENT} doesn't exist"
[[ -d ${SDDS_SSPL_OSQUERY_COMPONENT} ]]         || fail "Error: ${SDDS_SSPL_OSQUERY_COMPONENT} doesn't exist"
[[ -d ${SDDS_SSPL_MDR_COMPONENT} ]]             || fail "Error: ${SDDS_SSPL_MDR_COMPONENT} doesn't exist"
[[ -d ${SDDS_SSPL_MDR_COMPONENT_SUITE} ]]       || fail "Error: ${SDDS_SSPL_MDR_COMPONENT_SUITE} doesn't exist"
[[ -d ${SSPL_LIVERESPONSE_PLUGIN_SDDS} ]]       || fail "Error: ${SSPL_LIVERESPONSE_PLUGIN_SDDS} doesn't exist"
[[ -d ${WEBSOCKET_SERVER} ]]                    || fail "Error: ${WEBSOCKET_SERVER} doesn't exist"


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
