#!/bin/bash

# Ensure we run script from within sspl-tools directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
pushd "${SCRIPT_DIR}/../" &> /dev/null

# Do not run as root - we do not want the builds to be root owned
[[ $EUID -ne 0 ]] || error "Please do not run the script as root"

function error()
{
    sleep 0.5
	printf '\033[0;31m'
	echo "ERROR: $1"
	printf '\033[0m'
	exit 1
}

function prefixwith() {
    local prefix="$1"
    shift
    # Redirect stdout to sed command which appends prefix (and adds colour)
    "$@" > >(sed "s_^_\\o033[32mBuilding $prefix:\\o033[0m _")  2> >(sed "s_^_\\o033[32mBuilding $prefix:\\o033[0m _" >&2)
}


for buildScript in $(find . -name "build.sh")
do
    directory=$(dirname $buildScript)
    pushd $directory &> /dev/null

    chmod +x build.sh
    prefixwith "$directory" ./build.sh || error "Build of ${directory} failed!"
    popd &> /dev/null
done

popd &> /dev/null
