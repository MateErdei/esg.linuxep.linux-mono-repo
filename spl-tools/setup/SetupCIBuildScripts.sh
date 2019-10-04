#!/bin/bash

if [[ $(id -u) != 0 ]]
then
    echo "Please run this script as root."
    exit 0
fi

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )

mkdir -p ~/.config/pip/
pushd ~/.config/pip/
echo [global] > pip.conf
echo index-url = https://tap-artifactory1.eng.sophos/artifactory/api/pypi/pypi/simple >> pip.conf
echo cert = $SCRIPTDIR/sophos_certs.pem >> pip.conf

python3 -m pip install build_scripts
popd
rm -rf ~/.config/pip/

#Update the hardcoded paths to filer 5 and filer 6 in the build scripts to work on dev machines
BUILD_SCRIPT_INSTALL_DIR="$(python3 -m pip show build_scripts | grep "Location" | cut -d" " -f2)/build_scripts"

sed -i 's/\"\/mnt\/filer6\/\"/\"\/mnt\/filer6\/bfr\/\"/g' ${BUILD_SCRIPT_INSTALL_DIR}/build_common.py
sed -i 's/\/mnt\/filer\/bir/\/uk-filer5\/prodro\/bir/g' ${BUILD_SCRIPT_INSTALL_DIR}/build_common.py

#Create temporary location used by scripts
mkdir -p /SophosPackages
chmod 777 /SophosPackages

#Create or clear out the build folder used for unpacking gcc to
mkdir -p /build
chmod 777 /build
rm -rf /build/*

