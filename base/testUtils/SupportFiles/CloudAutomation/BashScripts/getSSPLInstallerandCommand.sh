#!/usr/bin/env bash
url=$1

if [[ $url != "https://api.sandbox.sophos/api/download/"* ]]; then
    echo "The url $url does not start with https://api.sandbox.sophos/api/download/"
    exit 1
fi
if [[ $url != *"SophosSetup.sh" ]]; then
    echo "The url $url does not point to the right installer, we need SophosSetup.sh"
    exit 1
fi
pushd /tmp
rm SophosSetup.sh
wget $url  --no-check-certificate

popd


file="/tmp/SophosSetup.sh"
if ! [[ -f $file ]]; then
    echo "installer SophosSetup.sh doesn't exist in tmp"
    exit 1
fi
TOKEN=$(grep -a TOKEN= $file | grep -v CLOUD | grep -v MCS | sed -r 's/^TOKEN=//')
URL=$(grep -a URL= $file | grep -v CLOUD | grep -v MCS | sed -r 's/^URL=//')
echo "/opt/sophos-spl/base/bin/registerCentral" $TOKEN $URL >> "/tmp/registerCommand"

if ! [[ -f "/tmp/registerCommand" ]]; then
    echo "register command not found"
    exit 1
fi
