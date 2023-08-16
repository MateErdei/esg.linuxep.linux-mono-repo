#!/usr/bin/env bash

set -e

function make_links()
{
  ln -sf "/home/$USER/gitrepos/esg.linuxep.sspl-tools/tests/remoterobot-wsl.py" robot
  ln -sf "/home/$USER/gitrepos/esg.linuxep.sspl-tools/vagrant" vagrant
}

# Base
if [ -d  /home/$USER/gitrepos/esg.linuxep.everest-base ]
then
  cd "/home/$USER/gitrepos/esg.linuxep.everest-base"
  make_links
  cd "/home/$USER/gitrepos/esg.linuxep.everest-base/testUtils"
  make_links
fi

# AV
if [ -d  /home/$USER/gitrepos/esg.linuxep.sspl-plugin-anti-virus ]
then
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-plugin-anti-virus"
  make_links
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-plugin-anti-virus/TA"
  make_links
fi

# EDR
if [ -d  /home/$USER/gitrepos/esg.linuxep.sspl-plugin-edr-component ]
then
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-plugin-edr-component"
  make_links
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-plugin-edr-component/TA"
  make_links
fi

# Event Journaler
if [ -d  /home/$USER/gitrepos/esg.linuxep.sspl-plugin-event-journaler ]
then
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-plugin-event-journaler"
  make_links
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-plugin-event-journaler/TA"
  make_links
fi

# Warehouse
if [ -d  /home/$USER/gitrepos/esg.linuxep.sspl-warehouse ]
then
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-warehouse"
  ln -sf "/home/$USER/gitrepos/esg.linuxep.sspl-tools/tests/remote_tap.py" tap
  ln -sf "/home/$USER/gitrepos/esg.linuxep.sspl-tools/vagrant" vagrant
fi
