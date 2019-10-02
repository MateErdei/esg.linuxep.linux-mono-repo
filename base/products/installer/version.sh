#!/bin/bash

shopt -s nullglob

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)
SOPHOS_INSTALL="${ABS_SCRIPTDIR%/bin*}"

base_version_file=${SOPHOS_INSTALL}/base/VERSION.ini
sophosmtr_plugin_version=${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini


echo ""
echo "Version information for Sophos Server Protection For Linux"
echo ""
if [[ -f ${base_version_file} ]]
then
	cat ${base_version_file}
fi

	echo ""

if [[ -f ${sophosmtr_plugin_version} ]]
then
	cat ${sophosmtr_plugin_version}
fi
