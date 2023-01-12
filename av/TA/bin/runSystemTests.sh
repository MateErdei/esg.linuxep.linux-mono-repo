#!/bin/bash

set -ex

export MCS_REGION=${MCS_REGION:-DEV}

SCRIPT_DIR=$PWD

echo "Test scripts from git branch: $GIT_BRANCH"
VUT_BRANCH=${VUT_BRANCH:-master}
echo "Product version under test from $VUT_BRANCH"

apt-get update
apt-get install -y libcap-dev python3.7 python3.7-dev python3.7-venv

# Create and enter TAP Python Virtual Environment
python3.7 -m venv tapvenv
. ./tapvenv/bin/activate

export PYTHONUNBUFFERED=1

# Get Build
python3.7 ${SCRIPT_DIR}/TA/bin/downloadBuild.py ${SCRIPT_DIR}/download ${VUT_BRANCH}
#ls -l ${SCRIPT_DIR}/download ${SCRIPT_DIR}/download/output

#ls -l $SCRIPT_DIR
#env

python3.7 -m pip install --upgrade keyrings.alt 2>/dev/null
python3.7 -m pip install pip --upgrade
python3.7 -m pip install -i https://artifactory.sophos-ops.com/api/pypi/pypi/simple -r TA/requirements.txt

# Setup test env
[ -d /opt/test ] && rm -rf /opt/test
mkdir -p /opt/test/inputs/av
ln -s $SCRIPT_DIR/TA /opt/test/inputs/test_scripts
OUTPUT=${SCRIPT_DIR}/download/output
ln -s $OUTPUT/test-resources /opt/test/inputs/av/test-resources
ln -s $OUTPUT/SDDS-COMPONENT /opt/test/inputs/av/SDDS-COMPONENT
ln -s $OUTPUT/base-sdds /opt/test/inputs/av/base-sdds

#ls -l /opt/test/inputs/av /opt/test/inputs/av/base-sdds/ /opt/test/inputs/av/SDDS-COMPONENT/ /opt/test/inputs/av/test-resources/

cd /opt/test/inputs/test_scripts

## run system tests
[ -f /opt/sophos-spl/bin/uninstall.sh ] && /opt/sophos-spl/bin/uninstall.sh --force
python3.7 -m robot -x robot.xml --include system .
cd $SCRIPT_DIR

if [[ -d /opt/sophos-spl ]]
then
	/opt/sophos-spl/bin/uninstall.sh --force
fi
