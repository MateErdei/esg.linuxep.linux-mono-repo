#!/bin/bash 

echo "Bootstrapping Ubuntu"
sudo apt-get update --fix-missing
sudo apt-get install build-essential cmake python-pip --assume-yes
sudo pip install robotframework pyzmq watchdog protobuf


