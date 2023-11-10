#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

REPO_PATH="/vagrant/esg.linuxep.linux-mono-repo"
DEVICE_ISOLATION_PATH="$REPO_PATH/deviceisolation"
BASE_PATH="$REPO_PATH/base"
COMMON_PATH="$REPO_PATH/common"
TEST_INPUTS="/opt/test/inputs"
FAKE_MGMT="$TEST_INPUTS/fake_management"

if [ ! -d "$DEVICE_ISOLATION_PATH" ]
then
  echo "Failed to find Device Isolation path: $DEVICE_ISOLATION_PATH"
  exit 1
fi

if [ ! -d "$BASE_PATH" ]
then
  echo "Failed to find Base path: $BASE_PATH"
  exit 1
fi

if [ ! -d "$FAKE_MGMT" ]
then
  mkdir -p $FAKE_MGMT
  unzip "$REPO_PATH/.output/base/fake_management.zip" -d $FAKE_MGMT
fi

ln -snf "$REPO_PATH/.output/deviceisolation/linux_x64_rel/installer" "$TEST_INPUTS/device_isolation_sdds"
ln -snf "$DEVICE_ISOLATION_PATH/TA"  "$TEST_INPUTS/test_scripts"
ln -snf "$REPO_PATH/.output/base/linux_x64_rel/installer" "$TEST_INPUTS/base_sdds"
ln -snf "$COMMON_PATH/TA/libs" "$TEST_INPUTS/common_test_libs"
ln -snf "$COMMON_PATH/TA/robot" "$TEST_INPUTS/common_test_robot"
ln -snf "$BASE_PATH/testUtils/SupportFiles" "$TEST_INPUTS/SupportFiles"

apt-get -y install python3 python3-pip
python3 -m pip install -r "$DEVICE_ISOLATION_PATH/TA/requirements.txt"

# Remove markers from other runs because at the moment they all share "/opt/test/inputs/test_scripts"
rm -f /tmp/ran_*_setup_vagrant_script
touch /tmp/ran_device_isolation_setup_vagrant_script
