#!/bin/bash

set -x

if which yum &> /dev/null
then
    yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm &> /dev/null
    yum install sshpass --assumeyes
    yum install wget --assumeyes
else
    apt-get update
    apt-get install sshpass --assume-yes
    apt-get install wget --assume-yes
fi