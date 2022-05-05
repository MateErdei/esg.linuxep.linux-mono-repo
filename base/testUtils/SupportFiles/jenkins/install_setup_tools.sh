#!/bin/bash

function install_setup_packages
{
  local pkg_manager
  if [ -n "$(which apt-get)" ]
    then
      pkg_manager="apt-get"
      list=p7zip-full
  elif [ -n "$(which yum)" ]
    then
      pkg_manager="yum"
      list=p7zip
  else
    echo "System is not rhel-based or ubuntu, cannot install packages"
    exit 1
  fi

  local installed_ok=0
  for (( i=1; i<=20; i++ ))
  do
    if sudo $pkg_manager -y install $list
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

install_setup_packages