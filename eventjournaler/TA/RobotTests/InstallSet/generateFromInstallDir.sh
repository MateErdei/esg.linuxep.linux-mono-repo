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


find /opt/sophos-spl -type d -exec stat -c '%a, %G, %U, %n' {} + | sort > DirectoryInfo
find /opt/sophos-spl -type f -exec stat -c '%a, %G, %U, %n' {} + | grep -vFf ExcludeFiles | sort > FileInfo
find /opt/sophos-spl -type l -exec stat -c '%a, %G, %U, %n' {} + | sort > SymbolicLinkInfo
if [[ -d /lib/systemd/system ]]
then
    find /lib/systemd/system/ -name sophos-spl* -type f -exec stat -c '%a, %G, %U, %n' {}  + | sort > SystemdInfo
elif [[ -d /usr/lib/systemd/system ]]
then
    find /usr/lib/systemd/system/ -name sophos-spl* -type f -exec stat -c '%a, %G, %U, %n' {}  + | sort > SystemdInfo
else
    echo "Could not find SSPL systemd directory."
    exit 1
fi

StripLastReturn "DirectoryInfo"
StripLastReturn "FileInfo"
StripLastReturn "SymbolicLinkInfo"
StripLastReturn "SystemdInfo"
