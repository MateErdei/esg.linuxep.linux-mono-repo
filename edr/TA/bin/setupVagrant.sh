#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

REPO_PATH="/vagrant/esg.linuxep.linux-mono-repo"
EDR_PATH="$REPO_PATH/edr"
BASE_PATH="$REPO_PATH/base"
COMMON_PATH="$REPO_PATH/common"

if [ ! -d "$EDR_PATH" ]
then
  echo "Failed to find EDR path: $EDR_PATH"
  exit 1
fi

if [ ! -d "$BASE_PATH" ]
then
  echo "Failed to find Base path: $BASE_PATH"
  exit 1
fi

mkdir -p /opt/test/inputs/edr/
ln -snf "$EDR_PATH/output/SDDS-COMPONENT" /opt/test/inputs/edr_sdds
ln -snf "$EDR_PATH/TA"  /opt/test/inputs/test_scripts
ln -snf "$EDR_PATH/local_test_input/qp"  /opt/test/inputs/qp
ln -snf "$EDR_PATH/local_test_input/lp"  /opt/test/inputs/lp
ln -snf "$BASE_PATH/output/SDDS-COMPONENT" /opt/test/inputs/base_sdds
ln -snf "$COMMON_PATH/TA/libs" /opt/test/inputs/common_test_libs
ln -snf "$COMMON_PATH/TA/robot" /opt/test/inputs/common_test_robot
apt-get -y install python3 python3-pip python3-pkgconfig
python3 -m pip install -r "$EDR_PATH/TA/requirements.txt"

# Remove markers from other runs because at the moment they all share "/opt/test/inputs/test_scripts"
rm -f /tmp/ran_*_setup_vagrant_script
touch /tmp/ran_edr_setup_vagrant_script
