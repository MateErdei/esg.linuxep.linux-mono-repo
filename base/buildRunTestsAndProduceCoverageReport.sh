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
echo "build Run Tests and Produce Coverge Report.sh with systemtests: ${SYSTEM_TEST}"
python3 -m build_scripts.artisan_fetch build/release-package.xml
./build.sh --python-coverage
SDDS_COMPONENT="${BASE}/output/SDDS_COMPONENT"
echo "Keep the coverage for unit tests"
cp modules/.coverage  unit_tests_coverage
pushd ${SYSTEM_TEST}
RERUNFAILED=true BASE_SOURCE="${SDDS_COMPONENT}" bash SupportFiles/jenkins/jenkinsBuildCommand.sh -i SMOKE -t 'MCS Status Sent When Message Relay Changed'
${SDDS_COMPONENT}/pyCoverage combine /tmp/register_central* /tmp/mcs_router*
sed -i "s_/opt/sophos-spl/base/lib64_${BASE}/modules/mcs_router_g" .coverage
cp .coverage ${BASE}/system_tests_coverage
popd$
${SDDS_COMPONENT}/pyCoverage combine unit_tests_coverage system_tests_coverage
${SDDS_COMPONENT}/pyCoverage xml
