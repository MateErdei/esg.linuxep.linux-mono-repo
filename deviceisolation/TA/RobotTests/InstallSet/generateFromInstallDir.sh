#!/usr/bin/env bash

function StripLastReturn()
{
    local fileName=$1
    cat $fileName | tr '\n' '_' | grep -q "_$"
    if [ $? -eq 0 ]
    then
       truncate -s -1 $fileName
    fi
}


if [ ! -d /opt/sophos-spl ]
then
    echo "Please install SSPL before running this"
    exit 1
fi


find /opt/sophos-spl/plugins/deviceisolation/ -type d -exec stat -c 'o%a, %G, %U, %n' {} + | sort > DirectoryInfo
find /opt/sophos-spl/plugins/deviceisolation/ -type f -exec stat -c 'o%a, %G, %U, %n' {} + | grep -vFf ExcludeFiles | sort > FileInfo
find /opt/sophos-spl/plugins/deviceisolation/ -type l -exec stat -c 'o%a, %G, %U, %n' {} + | sort > SymbolicLinkInfo

StripLastReturn "DirectoryInfo"
StripLastReturn "FileInfo"
StripLastReturn "SymbolicLinkInfo"
