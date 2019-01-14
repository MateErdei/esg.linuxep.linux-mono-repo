#!/bin/bash 

echo "Bootstrapping Ubuntu"
sudo mkdir -p /redist
grep allegro /etc/fstab > /dev/null
if [[ "x$?" != 'x0' ]]
then
   echo 'setup redist entry'
   cp /etc/fstab /tmp/fstab
   echo 'allegro:/redist /redist nfs ro,vers=3,tcp,exec 0 0
allegro:/oldTarFiles /oldTarFiles nfs ro,soft,intr,users 0 0
   ' >> /tmp/fstab
   sudo mv /tmp/fstab /etc/fstab
fi

if [[ ! -d /redist/binaries ]]
then
  sudo mount /redist
fi

if [[ ! -d /oldTarFiles ]]
then
  sudo mkdir -p /oldTarFiles
  sudo mount /oldTarFiles
fi


sudo mkdir -p /mnt/filer6/bfr
grep filer6 /etc/fstab > /dev/null
if [[ "x$?" != 'x0' ]]
then
   echo 'add ukfiler6 mount'
   cp /etc/fstab /tmp/fstab
   echo '//UK-FILER6.ENG.SOPHOS/BFR  /mnt/filer6/bfr cifs noauto,users,rw,domain=GREEN,username=qabuilduser,password=SalamiHat1   0   0' >> /tmp/fstab
   sudo mv /tmp/fstab /etc/fstab
fi
if [[ ! -d /mnt/filer6/bfr/ssp ]]
then
  sudo mount /mnt/filer6/bfr
fi

sudo apt-get update --fix-missing
sudo apt-get install build-essential cmake python-pip auditd --assume-yes
sudo pip install robotframework pyzmq watchdog protobuf paramiko

