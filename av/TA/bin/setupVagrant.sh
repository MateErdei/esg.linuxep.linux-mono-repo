#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

STASH_PATH="/vagrant/sspl-plugin-anti-virus"
GITHUB_PATH="/vagrant/esg.linuxep.linux-mono-repo/av"

if [ -d "$GITHUB_PATH" ]
then
  AV_PATH="$GITHUB_PATH"
elif [ -d "$STASH_PATH" ]
then
  AV_PATH="$STASH_PATH"
else
  echo "Failed to find AV path"
  exit 1
fi

mkdir -p /opt/test/inputs/av/
ln -snf "${AV_PATH}/TA/" /opt/test/inputs/test_scripts
ln -snf "${AV_PATH}/output/SDDS-COMPONENT/" /opt/test/inputs/av/SDDS-COMPONENT
ln -snf "${AV_PATH}/output/test-resources/" /opt/test/inputs/av/test-resources
ln -snf "${AV_PATH}/output/base-sdds/" /opt/test/inputs/av/base-sdds
ln -snf "${AV_PATH}/output/" /opt/test/inputs/tap_test_output_from_build

apt-get -y install python3 python3-pip python3-pkgconfig
apt-get -y install nfs-kernel-server samba p7zip-full gdb
python3 -m pip install -r requirements.txt

# Stop samba related services as they both start up on install
systemctl stop nmbd.service
systemctl stop smbd.service

# Remove markers from other runs because at the moment they all share "/opt/test/inputs/test_scripts"
rm -f /tmp/ran_*_setup_vagrant_script
touch /tmp/ran_av_setup_vagrant_script
