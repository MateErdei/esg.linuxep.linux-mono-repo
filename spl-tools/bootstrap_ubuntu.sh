#!/bin/bash

set -x

echo "Bootstrapping Ubuntu"
apt-get update --fix-missing
apt-get upgrade -y
apt-get install build-essential gdb cmake python3-pip auditd libc6-i386 clang python3.8-venv cifs-utils --assume-yes

# For guest additions
apt-get install -y libxt6 libxmu6

pip3 install pycapnp  pyzmq  pytest-html sseclient aiohttp_sse aiohttp robotframework psutil

# For base tests - TODO LINUXDAR-4150 - we should use requirements.txt
pip3 install pyzmq==19.0.1 protobuf==3.14.0 pytest-html psutil sseclient asyncio aiohttp==3.8.1 aiohttp_sse python-dateutil websockets robotframework watchdog proxy.py

echo "Done running ubuntu bootstrap script"