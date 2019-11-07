#!/bin/bash

echo "Bootstrapping Ubuntu"

apt-get update --fix-missing
apt-get install build-essential cmake python-pip python3-pip auditd libc6-i386 clang --assume-yes
pip2 install robotframework pyzmq protobuf paramiko pycapnp watchdog psutil kittyfuzzer katnip

cp /home/vagrant/auditdConfig.txt /vagrant