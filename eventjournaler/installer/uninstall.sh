#!/usr/bin/env bash

# Copyright 2018 Sophos Limited.  All rights reserved.

# Find the location of where the script is running from
# Allows the script to be called from anywhere either directly or via symlink

SCRIPT_LOCATION="${BASH_SOURCE[0]}"

# if file is symbolic link
if [ -h "$SCRIPT_LOCATION" ]
then
  SCRIPT_DIR="$( cd -P "$( dirname "$SCRIPT_LOCATION" )" >/dev/null && pwd )"
  SCRIPT_LOCATION="$(readlink "$SCRIPT_LOCATION")"  # get location of linked file

  [[ $SCRIPT_LOCATION != /* ]] && SCRIPT_LOCATION="$SCRIPT_DIR/$SCRIPT_LOCATION"
fi

SCRIPT_DIR="$( cd -P "$( dirname "$SCRIPT_LOCATION" )" >/dev/null && pwd )"

BASEDIR=${SCRIPT_DIR%%/plugins/ExamplePlugin/bin}

UNINSTALLDIR=$BASEDIR/plugins/ExamplePlugin
SYMLINK=$BASEDIR/base/update/var/installedproducts/ServerProtectionLinux-Plugin-Example.sh

"${BASEDIR}/bin/wdctl" removePluginRegistration "ExamplePlugin"

rm -rf "$UNINSTALLDIR"
rm -r "$SYMLINK"