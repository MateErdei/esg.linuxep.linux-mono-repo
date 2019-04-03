#!/bin/bash

echo "Bootstrapping Ubuntu"
mkdir -p /redist
grep allegro.eng.sophos /etc/fstab > /dev/null
if [[ "x$?" != 'x0' ]]
then
   echo 'setup redist entry'
   cp /etc/fstab /tmp/fstab
   echo 'allegro.eng.sophos:/redist /redist nfs ro,vers=3,tcp,exec 0 0
allegro.eng.sophos:/oldTarFiles /oldTarFiles nfs ro,soft,intr,users 0 0
   ' >> /tmp/fstab
   mv /tmp/fstab /etc/fstab
fi

if [[ ! -d /redist/binaries ]]
then
  mount /redist
fi

if [[ ! -d /oldTarFiles ]]
then
  mkdir -p /oldTarFiles
  mount /oldTarFiles
fi


mkdir -p /mnt/filer6/bfr
grep filer6 /etc/fstab > /dev/null
if [[ "x$?" != 'x0' ]]
then
   echo 'add ukfiler6 mount'
   cp /etc/fstab /tmp/fstab
   echo '//UK-FILER6.ENG.SOPHOS/BFR  /mnt/filer6/bfr cifs noauto,users,rw,domain=GREEN,username=qabuilduser,password=SalamiHat1   0   0' >> /tmp/fstab
   mv /tmp/fstab /etc/fstab
fi
if [[ ! -d /mnt/filer6/bfr/ssp ]]
then
  mount /mnt/filer6/bfr
fi

apt-get update --fix-missing
apt-get install build-essential cmake python-pip auditd libc6-i386 --assume-yes
pip2 install robotframework pyzmq protobuf paramiko pycapnp watchdog psutil kittyfuzzer katnip

cp /home/vagrant/auditdConfig.txt /vagrant