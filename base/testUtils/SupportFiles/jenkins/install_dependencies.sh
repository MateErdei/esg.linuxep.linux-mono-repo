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

function install_system_packages
{
  local pkg_manager
  if [ -n "$(which apt-get)" ]
    then
      pkg_manager="apt-get"
  elif [ -n "$(which yum)" ]
    then
      pkg_manager="yum"
  else
    echo "System is not rhel-based or ubuntu, cannot install packages"
    exit 1
  fi

  local installed_ok=0
  for (( i=1; i<=20; i++ ))
  do
    if sudo $pkg_manager -y install $@
    then
      installed_ok=1
      break
    else
       sleep 3
     fi
  done

  if [ $installed_ok = 0 ]
  then
    echo "Package install failed"
    exit 1
  fi
}

python3 -m pip install pip --upgrade

# install dependencies
for python_package in sseclient aiohttp aiohttp_sse asyncio nest_asyncio python-dateutil websockets packaging protobuf==3.14.0 psutil
do
  pip_install ${python_package}
done

install_system_packages zip unzip