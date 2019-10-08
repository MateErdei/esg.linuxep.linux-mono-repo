#!/bin/bash
set -ex

# assumes this is executed from everest-base/
BASE=$(pwd)
SYSTEM_TEST=""
while [[ $# -ge 1 ]]
do
    case $1 in
        --system-test)
          shift
          SYSTEM_TEST="$1"
          ;;
    esac
    shift
done
echo 'remove previous coverage results'
rm -rf modules/.coverage unit_tests_coverage system_tests_coverage /tmp/register_central* /tmp/mcs_router*
echo "build Run Tests and Produce Coverge Report.sh with systemtests: ${SYSTEM_TEST}"
python3 -m build_scripts.artisan_fetch build/release-package.xml
./build.sh --python-coverage
SDDS_COMPONENT="${BASE}/output/SDDS-COMPONENT"
echo "Keep the coverage for unit tests"
cp modules/.coverage  unit_tests_coverage
pushd ${SYSTEM_TEST}
echo 'run system tests'
RERUNFAILED=true BASE_SOURCE="${SDDS_COMPONENT}" bash SupportFiles/jenkins/jenkinsBuildCommand.sh -i SMOKE -t 'MCS Status Sent When Message Relay Changed'
echo 'combine system tests results'
sudo chmod a+wr /tmp/register_central* /tmp/mcs_router*
${SDDS_COMPONENT}/pyCoverage combine /tmp/register_central* /tmp/mcs_router*
echo 'replace name to the original source'
sed -i "s_/opt/sophos-spl/base/lib64_${BASE}/modules/mcs_router_g" .coverage
cp .coverage ${BASE}/system_tests_coverage
popd$
echo 'combine coverage results and publish it '
${SDDS_COMPONENT}/pyCoverage combine unit_tests_coverage system_tests_coverage
${SDDS_COMPONENT}/pyCoverage xml
