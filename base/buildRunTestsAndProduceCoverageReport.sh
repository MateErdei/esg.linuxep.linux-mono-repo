#!/bin/bash
set -ex
function failure() {
    echo >&2 $1
    exit 1
}
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
[[ -d ${SYSTEM_TEST} ]] || failure "Invalid path for system tests. "
echo 'remove previous coverage results'
rm -rf modules/.coverage
echo "build Run Tests and Produce Coverge Report.sh with systemtests: ${SYSTEM_TEST}"
git checkout build/release-package.xml
# FIXME LINUXDAR-749: Jenkins fails to find the dev package in filer6.
sed  -i 's#package buildtype="dev" name="sspl-telemetry-config-dev" version="1.0"#package buildtype="dev" name="sspl-telemetry-config" version="1.0/EES-9377"#' build/release-package.xml
DEPLOYMENT_TYPE="dev" python3 -m build_scripts.artisan_fetch build/release-package.xml
./build.sh --python-coverage
SDDS_COMPONENT="${BASE}/output/SDDS-COMPONENT"
echo "Keep the coverage for unit tests"
pushd modules
python3 -m coverage combine || echo 'ignore error'
[[ -f .coverage ]] && mv .coverage  ${SYSTEM_TEST}/.coverage
popd

pushd testUtils
echo 'run system tests'
TESTS2RUN="-e AMAZON_LINUX -i SMOKE"
USER=$(whoami)
if [[ ${USER} == "jenkins" ]]; then
  BASE_SOURCE="${SDDS_COMPONENT}" bash SupportFiles/jenkins/jenkinsBuildCommand.sh  ${TESTS2RUN} || echo "Test failure does not prevent the coverage report. "
  sudo chown ${USER} .coverage
else
  ./robot ${TESTS2RUN}
fi
echo 'replace path to the original source'

sed -i "s#/opt/sophos-spl/base/lib64#${BASE}/modules/mcsrouter#g" .coverage
sed -i "s/py.0/py/g" .coverage
sed -i "s_/opt/sophos-spl/base/lib_${SDDS_COMPONENT}/files/base/lib_g" .coverage

# create the xml report that is used by jenkins
python3 -m coverage xml -i  --omit="*python3.7*,*site-packages*"
# publish the report to filer 6

if [[ ${USER} == "jenkins" ]]; then
  TARGET_PATH=/mnt/filer6/linux/SSPL/testautomation/pythoncoverage/
  sudo cp .coverage ${TARGET_PATH}/latest_python_coverage
fi
popd
