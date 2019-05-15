#!/usr/bin/env bash

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

update ssh://git@stash.sophos.net:7999/linuxep/everest-base.git sspl-base
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
