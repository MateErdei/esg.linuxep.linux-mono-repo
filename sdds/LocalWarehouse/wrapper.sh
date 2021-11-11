#!/bin/bash

echo "See README.txt for documentation"
echo "Installing dependencies"


echo "----------------------------------------------------------------------------------------"
echo "[!] You MUST run this script as '. ./setupEnvironment.sh' to set the environment variables"
echo "----------------------------------------------------------------------------------------"
sleep 1

for i in "$@"
do
  case $i in
    --offline-mode)
      OFFLINE_MODE=1
      echo "Running in offline mode"
      sleep 1
    ;;
    --help)
      python3 ./generate_local_warehouse.py --help
      STOP=1
  esac
done

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root"
   STOP=1
fi


function install_dependencies()
{
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
}

[[ -z $OFFLINE_MODE && -z $STOP ]] && install_dependencies
#incase this script is run again in the current shell
unset OFFLINE_MODE

function done_message()
{
  echo "Attempting to set: OVERRIDE_SOPHOS_LOCATION=https://localhost:8000"
  echo "rerun this script or export the above environment variable again if you need to set the environment variable again"
  echo "----------------------------------------------------------------------------------------"
  echo "DONE!"
  echo "To Install VUT of selected warehouse branch (default is develop):"
  echo -e "\tUsername: av_user_vut"
  echo -e "\tPassword: Password"
  echo -e "\tAddress: https://localhost:8000"
  echo -e "export OVERRIDE_SOPHOS_LOCATION=https://localhost:8000"
  echo "To Install 999 of selected warehouse branch (default is develop):"
  echo -e "\tUsername: av_user_999"
  echo -e "\tPassword: Password"
  echo -e "\tAddress: https://localhost:8001"
  echo -e "export OVERRIDE_SOPHOS_LOCATION=https://localhost:8001"
  export OVERRIDE_SOPHOS_LOCATION=https://localhost:8000
}

[[ -z $STOP ]] && python3 ./generate_local_warehouse.py "$@" && done_message

