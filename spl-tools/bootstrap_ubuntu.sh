#!/bin/bash

echo "Bootstrapping Ubuntu"

grep filer6 /etc/fstab > /dev/null
if [[ "x$?" != 'x0' ]]
then
   echo "Please clear your cached vagrant box, and get latest from filer6 by running 'vagrant destroy <box-name>' and 'vagrant box remove <box - name>'"
fi

apt-get update --fix-missing
apt-get install build-essential cmake python-pip auditd libc6-i386 --assume-yes
pip2 install robotframework pyzmq protobuf paramiko pycapnp watchdog psutil kittyfuzzer katnip

cp /home/vagrant/auditdConfig.txt /vagrant