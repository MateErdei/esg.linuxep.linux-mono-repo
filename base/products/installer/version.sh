#!/bin/bash

base_version_file=../base/VERSION.ini
sophosmtr_plugin_version=../plugins/mtr/VERSION.ini


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
