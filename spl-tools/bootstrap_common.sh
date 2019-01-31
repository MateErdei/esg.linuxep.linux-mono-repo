#!/bin/bash

if which yum &> /dev/null
then
    sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm &> /dev/null
    sudo yum install sshpass --assumeyes
    sudo yum install wget --assumeyes
else
    sudo apt-get install sshpass --assume-yes
    sudo apt-get install wget --assume-yes
fi