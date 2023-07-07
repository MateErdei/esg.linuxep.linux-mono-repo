#!/bin/bash

# Copyright 2023 Sophos Limited.  All rights reserved.

# script to check if a breaking upgrade patch needs to be applied,
# if a patch is found it is run

VERSION=""
[[ -n "$SOPHOS_INSTALL" ]] || SOPHOS_INSTALL=/opt/sophos-spl

# pass in the path of installed version.ini, location of installset and rigid name of product
function check_for_upgrade()
{
  VERSIONINI=$1
  PRODUCT_LINE_ID=$2
  DIST=$3

  if [[ ! -f "$VERSIONINI" ]]
  then
      # this is fresh install
      return 0
  fi

  get_version  "$VERSIONINI" || return 1

  UPGRADE_SCRIPT=${DIST}/upgrade/${VERSION}.sh
  # check if there is an upgrade script for this version and if write file for the installer to read with its path
  if [[ -f "$UPGRADE_SCRIPT" ]]
  then
      echo "Running upgrade script $UPGRADE_SCRIPT"
      chmod +x "$UPGRADE_SCRIPT"
      "$UPGRADE_SCRIPT"
  fi
}

function get_version()
{
  VERSIONINI=$1
  PRODUCT_VERSION_LINE=""
  while IFS= read -r line; do
      if [[ $line == "PRODUCT_VERSION = "* ]]
      then
        PRODUCT_VERSION_LINE=${line#"PRODUCT_VERSION = "}
        break
      fi
  done < "$VERSIONINI"

  if [[ ! -n "$PRODUCT_VERSION_LINE" ]]
  then
    echo "$VERSIONINI is malformed it does not contain a product version"
    return 1
  fi

  VERSION=${PRODUCT_VERSION_LINE%.*}
  return 0
}
