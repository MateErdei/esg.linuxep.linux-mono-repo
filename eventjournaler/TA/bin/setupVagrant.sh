#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

REPO_PATH="/vagrant/esg.linuxep.linux-mono-repo"
JOURNALER_PATH="$REPO_PATH/eventjournaler"
BASE_PATH="$REPO_PATH/base"
COMMON_PATH="$REPO_PATH/common"

if [ ! -d "$JOURNALER_PATH" ]
then
  echo "Failed to find Event Journaler path: $JOURNALER_PATH"
  exit 1
fi

if [ ! -d "$BASE_PATH" ]
then
  echo "Failed to find Base path: $BASE_PATH"
  exit 1
fi

mkdir -p /opt/test/inputs/event_journaler/
ln -snf "$JOURNALER_PATH/output/SDDS-COMPONENT" /opt/test/inputs/event_journaler_sdds
ln -snf "$JOURNALER_PATH/output/manualTools"  /opt/test/inputs/manual_tools
ln -snf "$JOURNALER_PATH/TA"  /opt/test/inputs/test_scripts
ln -snf "$JOURNALER_PATH/output/fake-management"  /opt/test/inputs/fake_management
ln -snf "$BASE_PATH/output/SDDS-COMPONENT" /opt/test/inputs/base_sdds
ln -snf "$COMMON_PATH/TA/libs" /opt/test/inputs/common_test_libs
ln -snf "$COMMON_PATH/TA/robot" /opt/test/inputs/common_test_robot

apt-get -y install python3 python3-pip
python3 -m pip install -r "$JOURNALER_PATH/TA/requirements.txt"

# Remove markers from other runs because at the moment they all share "/opt/test/inputs/test_scripts"
rm -f /tmp/ran_*_setup_vagrant_script
touch /tmp/ran_event_journaler_setup_vagrant_script
