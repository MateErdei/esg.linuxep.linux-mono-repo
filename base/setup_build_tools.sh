#!/bin/bash

#set -x
set -e

if [[ $(id -u) == 0 ]]
then
    echo "You don't need to run the entire script as root"
    exit 1
fi

TAP_VENV="tap_venv"

# Install python3 venv for TAP and then install TAP

if ! apt list python3-venv | grep -q python3-venv
then
  sudo apt-get install python3-venv -y
fi

if [ -d "$TAP_VENV" ]
then
 echo "$TAP_VENV already exists, not re-creating it"
 source "$TAP_VENV/bin/activate"
else
  python3 -m venv "$TAP_VENV"
  pushd "$TAP_VENV"
    echo "[global]" > pip.conf
    echo "index-url = https://tap-artifactory1.eng.sophos/artifactory/api/pypi/pypi/simple" >> pip.conf
    echo "trusted-host = tap-artifactory1.eng.sophos" >> pip.conf
  popd
  source "$TAP_VENV/bin/activate"
  echo "Using python:"
  which python
  python3 -m pip install wheel
  ##python3 -m pip install build_scripts
  python3 -m pip install keyrings.alt
  python3 -m pip install tap
fi

TAP_CACHE="/SophosPackages"
if [ -d "$TAP_CACHE" ]
then
  echo "TAP cache dir already exists: $TAP_CACHE"
else
  echo "Creating TAP cache dir: $TAP_CACHE"
  sudo mkdir "$TAP_CACHE"
  sudo chown "$USER" "$TAP_CACHE"
fi

tap --version
tap ls
tap fetch sspl_base.build.release

#TODO export CC and CXX, tell user this has ben done
