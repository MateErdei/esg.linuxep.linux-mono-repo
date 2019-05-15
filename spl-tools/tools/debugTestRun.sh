#!/usr/bin/env bash

set -ex

function failure()
{
    local EXIT=$1
    shift
    echo "$@" >&2
    exit $EXIT
}

function update()
{
    if [[ -d $2/.git ]]
    then
        pushd $2
        git pull
        popd
    else
        git clone "$1" "$2"
    fi
}
## Sort out SSH key
env | sort

[[ -f $GIT_SSH_KEYFILE ]] || failure 2 "Can't find git SSH keyfile: $GIT_SSH_KEYFILE"
export GIT_SSH_COMMAND="ssh -i ${GIT_SSH_KEYFILE}"

update ssh://git@stash.sophos.net:7999/linuxep/everest-base.git sspl-base
[[ -d sspl-base ]] || failure 1 "Failed to checkout Base"
update ssh://git@stash.sophos.net:7999/linuxep/thininstaller.git thininstaller
update ssh://git@stash.sophos.net:7999/linuxep/everest-systemproducttests.git sspl-systemtests
update ssh://git@stash.sophos.net:7999/linuxep/sspl-plugin-mdr-component.git sspl-plugin-mdr-component
update ssh://git@stash.sophos.net:7999/linuxep/sspl-plugin-mdr-component.git sspl-plugin-mdr-component
update ssh://git@stash.sophos.net:7999/linuxep/sspl-plugin-mdr-componentsuite.git sspl-plugin-mdr-componentsuite

mkdir -p /redist
[[ -d /redist/binaries ]] || mount -t nfs allegro.eng.sophos:/redist  /redist


cd sspl-base
./build.sh --debug
cd ..

## other builds


## Run system tests

cd sspl-systemtests
robot --exclude MANUAL tests
cd ..
