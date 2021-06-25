#!/usr/bin/env bash

set -x

function pip_install()
{
  local python_target="$1"
  python3 -c "import ${python_target}" 2> /dev/null
  if [[ "$?" != "0" ]];then
    sudo -E python3 -m pip install ${python_target}
  else
    echo "Python package ${python_target} already installed"
  fi
}

python3 -m pip install pip --upgrade

# install dependencies
for python_package in sseclient aiohttp aiohttp_sse asyncio python-dateutil websockets packaging protobuf==3.14.0
do
  pip_install ${python_package}
done

if [ -n "$(which apt-get)" ]
then
  sudo apt-get -y install zip unzip || ( echo "package install failed" && exit 1 )
elif [ -n "$(which yum)" ]
then
  sudo yum -y install zip unzip || ( echo "package install failed" && exit 1 )
else
  echo "System is not rhel-based or ubuntu, cannot install packages"
  exit 1
fi