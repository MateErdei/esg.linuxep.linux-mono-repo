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
}

function prefixwith() {
    local prefix="$1"
    shift
    # Redirect stdout to sed command which appends prefix (and adds colour)
    "$@" > >(sed "s_^_\\o033[32mBuilding $prefix:\\o033[0m _")  2> >(sed "s_^_\\o033[32mBuilding $prefix:\\o033[0m _" >&2)
}

failedBuilds=()

for repo in $(cat setup/gitRepos.txt)
do
    echo $repo 
    repoName=$(echo $repo | awk '{n=split($0, a, "/"); print a[n]}' | sed -n "s_\([^\.]*\).*_\1_p")
    ls $repoName
    pushd $repoName &> /dev/null

    if [[ -f build.sh ]] 
    then
        chmod +x build.sh
        prefixwith "$repoName" ./build.sh || failedBuilds+=($repoName)    
    fi
    popd &> /dev/null
done

for build in ${failedBuilds[@]}
do
    error "Build of ${build} failed!"
done

popd &> /dev/null
