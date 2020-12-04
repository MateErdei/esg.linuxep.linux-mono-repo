#!/bin/bash
set -ex
function failure() {
    echo >&2 $1
    exit 1
}
# assumes this is executed from everest-base/
BASE=$(pwd)

echo 'remove previous coverage results'
rm -rf modules/.coverage
echo "build Run Tests and Produce Coverge Report.sh with systemtests"
git checkout build/release-package.xml
sudo -H ./testUtils/SupportFiles/jenkins/SetupCIBuildScripts.sh
export PATH=$PATH:/usr/local/bin/

which sb_manifest_sign

./fetchandbuild.sh --python-coverage
SDDS_COMPONENT="${BASE}/output/SDDS-COMPONENT"
echo "Keep the coverage for unit tests"
pushd modules
python3 -m coverage combine || echo 'ignore error'
[[ -f .coverage ]] && mv .coverage  ../testUtils/.coverage
popd
#setup pycryptodome imports, system python only goes up to 3.6 and we build the product with 3.7
sudo rm -rf /usr/local/lib64/python3.6/site-packages/Crypto/*
sudo cp -r redist/pycryptodome/Crypto/*   /usr/local/lib64/python3.6/site-packages/Crypto/

pushd testUtils
echo 'run system tests'
TESTS2RUN="-e AMAZON_LINUX -i CENTRAL -i FAKE_CLOUD -i MCS -i MCS_ROUTER -i MESSAGE_RELAY -i REGISTRATION -i THIN_INSTALLER -i UPDATE_CACHE -e OSTIA"
USER=$(whoami)
if [[ ${USER} == "jenkins" ]]; then
#  COVERAGE_BASE_BUILD="${SDDS_COMPONENT}" bash SupportFiles/jenkins/jenkinsBuildCommand.sh  ${TESTS2RUN} || echo "Test failure does not prevent the coverage report. "
  sudo chown ${USER} .coverage
else
  ./robot ${TESTS2RUN}
fi
echo 'replace path to the original source'

sed -i "s#/opt/sophos-spl/base/lib64#${BASE}/modules/mcsrouter#g" .coverage
sed -i "s/py.0/py/g" .coverage
sed -i "s_/opt/sophos-spl/base/lib_${SDDS_COMPONENT}/files/base/lib_g" .coverage


find . -name '*coverage*' || echo "could not perform find"
# create the xml report that is used by jenkins
python3 -m coverage combine || echo 'ignore error'
python3 -m coverage xml -i  --omit="*python3.7*,*site-packages*,*build64*,*tests"
# publish the report to filer 6
find . -name "*coverage*"  || echo "could not perform find"
if [[ ${USER} == "jenkins" ]]; then
  TARGET_PATH=/mnt/filer6/linux/SSPL/testautomation/pythoncoverage/
  sudo cp .coverage ${TARGET_PATH}/latest_python_coverage
fi
popd
