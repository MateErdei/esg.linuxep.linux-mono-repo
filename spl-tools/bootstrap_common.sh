#!/bin/bash

if which yum &> /dev/null
then
    sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm &> /dev/null
    sudo yum install sshpass --assumeyes
else
    sudo apt-get install sshpass --assume-yes
fi