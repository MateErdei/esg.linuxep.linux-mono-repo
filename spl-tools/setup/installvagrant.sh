#!/bin/bash
# install vagrant if not installed

vagrant --version
if [ "x$?" != "x0" ]; then
  echo 'Installing vagrant'
  VAGRANTDEB=/mnt/filer6/linux/users/Gesner/vagrant_2.2.0_x86_64.deb
  sudo dpkg -i "$VAGRANTDEB"
else
  echo 'vagrant already installed'
fi
