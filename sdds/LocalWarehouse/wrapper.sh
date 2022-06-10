#!/bin/bash

echo "See README.txt for documentation"
echo "Installing dependencies"

TAP_PYPI="https://artifactory.sophos-ops.com/api/pypi/pypi/simple"

echo "----------------------------------------------------------------------------------------"
echo "[!] You MUST run this script as '. ./wrapper.sh' to set the environment variables"
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
      return 1 || echo "This script should be ran as '. ./wrapper.sh <args>'" && exit 1
  esac
done

if [[ -z $OFFLINE_MODE ]]
then
  curl $TAP_PYPI -kf
  RC=$?
  if [[ $RC != 0 ]]
  then
    echo "Failed to reach $TAP_PYPI. Check your network connection or try --offline-mode" && return 1 || echo "This script should be ran as '. ./wrapper.sh <args>'" && exit 1
  fi

fi

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root"
   return 1 || echo "This script should be ran as '. ./wrapper.sh <args>'" && exit 1
fi


function install_dependencies()
{
  echo "----------------------------------------------------------------------------------------"
  echo upgrading/installing git
  echo "----------------------------------------------------------------------------------------"
  if [ -n "$(which apt-get)" ]
  then
    apt update
    apt-get install git -y || echo failed to install git
    # ubuntu python is missing -m pip and -m ensurepip, use apt:
    apt-get install python3-pip -y || echo failed to install python3-pip
  elif [ -n "$(which yum)" ]
  then
    yum install git -y
  fi

  echo "----------------------------------------------------------------------------------------"
  echo installing/upgrading pip dependencies
  python3 -m pip install --upgrade pip -i $TAP_PYPI --trusted-host artifactory.sophos-ops.com
  # pip cannot implicitly handle old PyYAML, get a recent one: https://github.com/yaml/pyyaml/issues/349
  python3 -m pip install --ignore-installed PyYAML -i $TAP_PYPI --trusted-host artifactory.sophos-ops.com
  python3 -m pip install --upgrade tap keyrings.alt -i $TAP_PYPI --trusted-host artifactory.sophos-ops.com
  echo "----------------------------------------------------------------------------------------"
}

[[ -z $OFFLINE_MODE ]] && install_dependencies
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
  echo -e "\tPassword: password"
  echo -e "\tAddress: https://localhost:8000"
  echo -e "export OVERRIDE_SOPHOS_LOCATION=https://localhost:8000"
  echo "To Install 999 of selected warehouse branch (default is develop):"
  echo -e "\tUsername: av_user_999"
  echo -e "\tPassword: password"
  echo -e "\tAddress: https://localhost:8001"
  echo -e "export OVERRIDE_SOPHOS_LOCATION=https://localhost:8001"
  export OVERRIDE_SOPHOS_LOCATION=https://localhost:8000
}

python3 ./generate_local_warehouse.py "$@" && done_message

