#!/bin/bash
set -ex
function failure() {
    echo >&2 $1
    exit 1
}
# assumes this is executed from everest-base/

BASE=$(pwd)/base
pushd ${BASE}

echo 'remove previous coverage results'
rm -rf modules/.coverage*
rm -rf tests/mcsrouter/.coverage*
echo "build Run Tests and Produce Coverge Report.sh with systemtests"
git checkout build/release-package.xml
export PATH=$PATH:/usr/local/bin/
./tap_fetch.sh
./build.sh --release --python-coverage
SDDS_COMPONENT="${BASE}/output/SDDS-COMPONENT"
echo "Keep the coverage for unit tests"
pushd ${BASE}/tests/mcsrouter
python3 -m coverage combine || echo 'ignore error'
[[ -f .coverage ]] && mv .coverage  ../../testUtils/.coverage
popd

pushd ${BASE}/testUtils
echo 'run system tests'
TESTS2RUN="-e AMAZON_LINUX -i CENTRAL -i FAKE_CLOUD -i MCS -i MCS_ROUTER -i MESSAGE_RELAY -i REGISTRATION -e THIN_INSTALLER -i UPDATE_CACHE -e OSTIA"
USER=$(whoami)
if [[ ${USER} == "jenkins" ]]; then
  COVERAGE_BASE_BUILD="${SDDS_COMPONENT}" bash SupportFiles/jenkins/jenkinsBuildCommand.sh  ${TESTS2RUN} || echo "Test failure does not prevent the coverage report. "
  sudo chown ${USER} .coverage
else
  ./robot ${TESTS2RUN}
fi
echo 'replace path to the original source'

python3 "$BASE/testUtils/replacePythonCoveragePaths.py" -c "$BASE/testUtils/.coverage" -f "/opt/sophos-spl/base/lib64" -r "${BASE}/modules/mcsrouter"
python3 "$BASE/testUtils/replacePythonCoveragePaths.py" -c "$BASE/testUtils/.coverage" -f "py.0" -r "py"
python3 "$BASE/testUtils/replacePythonCoveragePaths.py" -c "$BASE/testUtils/.coverage" -f "/opt/sophos-spl/base/lib" -r "${SDDS_COMPONENT}/files/base/lib"

# create the xml report that is used by jenkins
python3 -m coverage combine || echo 'ignore error'
python3 -m coverage xml -i  --omit="*dist-packages*,*python3.7*,*site-packages*,*build64*,*tests"  -o "${BASE}/testUtils/coverage.xml"
# publish the report to filer 6
if [[ ${USER} == "jenkins" ]]; then
  TARGET_PATH=/mnt/filer6/linux/SSPL/testautomation/pythoncoverage/
  sudo cp .coverage ${TARGET_PATH}/latest_python_coverage
fi
popd
