#!/bin/bash 
# this script needs to be run as root. It installs dogfood sav
if [ -d /opt/sophos-av/ ]; then
  echo 'Sophos is already installed. If you want to re-install, please, uninstall it first' 
  exit 1
fi

echo 'copy installer from file 6' 
cp /mnt/filer6/linux/users/Gesner/SophosInstall.sh /tmp/

echo 'run installer'
/tmp/SophosInstall.sh
rm /tmp/SophosInstall.sh


echo 'copy dogfood feedback script'
cp /mnt/filer6/linux/users/RichardH/DogfoodLogCollector/savlinuxDogfoodFeedback.sh /opt/sophos-av/

echo 'setup dogfood feedback cron job'
crontab -l > /tmp/tmpcrontab
grep savlinuxDogfood /tmp/tmpcrontab > /dev/null
if [ "x$?" != "x0" ]; then
  echo '00 01 * * * /bin/bash /opt/sophos-av/savlinuxDogfoodFeedback.sh' >> /tmp/tmpcrontab
  crontab /tmp/tmpcrontab
  rm /tmp/tmpcrontab
else
  echo 'cronttab already setup'
fi
