#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

EDR_PATH="/vagrant/esg.linuxep.sspl-plugin-edr-component"
BASE_PATH="/vagrant/esg.linuxep.everest-base"

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
ln -snf "$EDR_PATH/local_test_input/lp_tar"  /opt/test/inputs/lp_tar
ln -snf "$EDR_PATH/local_test_input/qp"  /opt/test/inputs/qp
ln -snf "$BASE_PATH/output/SDDS-COMPONENT" /opt/test/inputs/base_sdds
apt-get -y install python3 python3-pip python3-pkgconfig
python3 -m pip install -r "$EDR_PATH/TA/requirements.txt"

# Remove markers from other runs because at the moment they all share "/opt/test/inputs/test_scripts"
rm /tmp/ran_*_setup_vagrant_script
touch /tmp/ran_edr_setup_vagrant_script
