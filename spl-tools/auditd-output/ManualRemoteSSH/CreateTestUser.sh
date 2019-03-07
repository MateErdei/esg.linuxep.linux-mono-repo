#!/bin/bash
./ClearLogs.sh
useradd testuser
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
mkdir -p /home/testuser/.ssh
touch /home/testuser/.ssh/authorized_keys
