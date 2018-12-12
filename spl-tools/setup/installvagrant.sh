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

vagrant plugin list | grep vagrant-aws
if [ "x$?" != "x0" ]; then
  echo 'Installing the vagrant-aws plugin'
  sudo vagrant plugin install vagrant-aws
else
  echo 'vagrant-aws plugin already installed'
fi

vagrant plugin list | grep vagrant-scp
if [ "x$?" != "x0" ]; then
  echo 'Installing the vagrant-scp plugin'
  sudo vagrant plugin install vagrant-scp
else
  echo 'vagrant-scp plugin already installed'
fi