#!/bin/bash
if [ ! -f Vagrantfile ]; then
  echo 'Please, run sspl-repositories from the sspl-tools root folder'; 
  exit 1
fi

if [ ! -d everest-base ]; then
  git clone ssh://git@stash.sophos.net:7999/linuxep/everest-base.git
fi

if [ ! -d exampleplugin ]; then
  git clone ssh://git@stash.sophos.net:7999/linuxep/exampleplugin.git
fi

if [ ! -d everest-systemproducttests ]; then
  git clone ssh://git@stash.sophos.net:7999/linuxep/everest-systemproducttests.git
fi

if [ ! -d thininstaller ]; then
  git clone ssh://git@stash.sophos.net:7999/linuxep/thininstaller.git
fi







