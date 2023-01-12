#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

mkdir -p /opt/test/inputs/av/
ln -snf /vagrant/sspl-plugin-anti-virus/TA/ /opt/test/inputs/test_scripts
ln -snf /vagrant/sspl-plugin-anti-virus/output/SDDS-COMPONENT/ /opt/test/inputs/av/SDDS-COMPONENT
ln -snf /vagrant/sspl-plugin-anti-virus/output/test-resources/ /opt/test/inputs/av/test-resources
ln -snf /vagrant/sspl-plugin-anti-virus/output/base-sdds/ /opt/test/inputs/av/base-sdds
ln -snf /vagrant/sspl-plugin-anti-virus/output/ /opt/test/inputs/tap_test_output_from_build

apt-get -y install python3.7 python3-pip python3-pkgconfig
apt-get -y install nfs-kernel-server samba p7zip-full gdb
python3.7 -m pip install -r requirements.txt
