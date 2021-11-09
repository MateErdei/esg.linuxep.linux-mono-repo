#!/bin/bash

echo "See README.txt for documentation"
echo "Installing dependencies"

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root"
   exit 1
fi

echo "----------------------------------------------------------------------------------------"
echo "[!] You MUST run this script as '. ./setupEnvironment.sh' to set the environment variables"
echo "----------------------------------------------------------------------------------------"
sleep  3

echo "----------------------------------------------------------------------------------------"
echo upgrading/installing git
echo "----------------------------------------------------------------------------------------"
if [ -n "$(which apt-get)" ]
then
  apt-get install git -y || echo failed to install git
elif [ -n "$(which yum)" ]
then
  yum install git -y
fi

echo "----------------------------------------------------------------------------------------"
echo installing/upgrading pip dependencies
pip3 install --upgrade pip -i https://tap-artifactory1.eng.sophos/artifactory/api/pypi/pypi/simple --trusted-host tap-artifactory1.eng.sophos
pip3 install --upgrade tap keyrings.alt --upgrade pip -i https://tap-artifactory1.eng.sophos/artifactory/api/pypi/pypi/simple --trusted-host tap-artifactory1.eng.sophos
echo "----------------------------------------------------------------------------------------"

python3 ./generate_local_warehouse.py $@

echo Setting: OVERRIDE_SOPHOS_LOCATION=https://localhost:8000
echo rerun this script or export the above environment variable again if you need to set the environment variable again
echo "----------------------------------------------------------------------------------------"
echo "DONE!"
echo "To Install VUT of selected warehouse branch (default is develop):"
echo -e "\tUsername: av_user_vut"
echo -e "\tPassword: Password"
echo -e "\tAddress: https://localhost:8000"
export OVERRIDE_SOPHOS_LOCATION=https://localhost:8000


