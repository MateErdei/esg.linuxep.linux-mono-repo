#!/usr/bin/env bash

set -e

THIS_FILE_FULL_PATH=$(readlink -f $0)
THIS_DIR=$(dirname "$THIS_FILE_FULL_PATH")
LINUX_MONO_REPO=$(dirname "$THIS_DIR")

function make_links()
{
  ln -sf "$THIS_DIR/tests/remoterobot-wsl.py" robot
  ln -sf "$THIS_DIR/vagrant" vagrant
}

# Linux mono repo
cd "$LINUX_MONO_REPO"
make_links

# Base
if [ -d  "$LINUX_MONO_REPO/base" ]
then
  cd "$LINUX_MONO_REPO/base"
  make_links
  cd "$LINUX_MONO_REPO/base/testUtils"
  make_links
fi

# AV
if [ -d  "$LINUX_MONO_REPO/av" ]
then
  cd "$LINUX_MONO_REPO/av"
  make_links
  cd "$LINUX_MONO_REPO/av/TA"
  make_links
fi

# EDR
if [ -d  "$LINUX_MONO_REPO/edr" ]
then
  cd "$LINUX_MONO_REPO/edr"
  make_links
  cd "$LINUX_MONO_REPO/edr/TA"
  make_links
fi

# Event Journaler
if [ -d  "$LINUX_MONO_REPO/eventjournaler" ]
then
  cd "$LINUX_MONO_REPO/eventjournaler"
  make_links
  cd "$LINUX_MONO_REPO/eventjournaler/TA"
  make_links
fi

# Warehouse
if [ -d  /home/$USER/gitrepos/esg.linuxep.sspl-warehouse ]
then
  cd "/home/$USER/gitrepos/esg.linuxep.sspl-warehouse"
  ln -sf "/home/$USER/gitrepos/esg.linuxep.linux-mono-repo/spl-tools/tests/remote_tap.py" tap
  ln -sf "/home/$USER/gitrepos/esg.linuxep.linux-mono-repo/spl-tools/vagrant" vagrant
fi
