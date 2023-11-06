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
        git fetch
        popd
    else
        git clone "$1" "$2"
    fi
    pushd $2
    if [[ -n $3 ]]
    then
        git checkout -f $3
    fi
    git pull
    popd
}
## Sort out SSH key
env | sort

[[ -f "$GIT_SSH_KEYFILE" ]] || failure 2 "Can't find git SSH keyfile: $GIT_SSH_KEYFILE"
export GIT_SSH_COMMAND="ssh -v -i ${GIT_SSH_KEYFILE}"
chmod 600 "$GIT_SSH_KEYFILE"
export GIT_TRACE=1
echo "${GIT_SSH_COMMAND} \$*" >/tmp/ssh-jenkins-git
chmod 700 /tmp/ssh-jenkins-git
export GIT_SSH=/tmp/ssh-jenkins-git

git --version

update ssh://git@stash.sophos.net:7999/linuxep/everest-base.git sspl-base
[[ -d sspl-base ]] || failure 1 "Failed to checkout Base"
update ssh://git@stash.sophos.net:7999/linuxep/thininstaller.git thininstaller
update ssh://git@stash.sophos.net:7999/linuxep/sspl-plugin-mdr-component.git sspl-plugin-mdr-component
update ssh://git@stash.sophos.net:7999/linuxep/sspl-plugin-mdr-componentsuite.git sspl-plugin-mdr-componentsuite


update ssh://git@stash.sophos.net:7999/linuxep/everest-systemproducttests.git sspl-systemtests ${SYSTEM_TEST_BRANCH:master}

sudo chown -R jenkins: .

mkdir -p /redist
[[ -d /redist/binaries ]] || mount -t nfs allegro.eng.sophos:/redist  /redist

ALLEGRO_INPUT=/redist/binaries/linux11/input

if [[ -d /build/input/gcc ]]
then
    export PATH=/build/input/gcc/bin:$PATH
    export LD_LIBRARY_PATH=/build/input/gcc/lib:/build/input/gcc/lib64:$LD_LIBRARY_PATH
fi

if [[ -d /redist/binaries/linux11/input/cmake ]]
then
    export PATH=${ALLEGRO_INPUT}/cmake/bin:$PATH
fi

STARTING_DIR=$(pwd)

pushd sspl-base
./build.sh --debug --noclean
popd

pushd sspl-plugin-mdr-component
./build.sh --debug --noclean
popd

pushd sspl-plugin-mdr-componentsuite
mkdir -p input
ln -snf ${STARTING_DIR}/sspl-plugin-mdr-component/output input/mdr_plugin
ln -snf ${ALLEGRO_INPUT}/dbos input/dbos
ln -snf ${ALLEGRO_INPUT}/osquery input/osquery
./build.sh
popd

## Run system tests
export SYSTEM_PRODUCT_TEST_OUTPUT=${STARTING_DIR}/sspl-base/output
export OUTPUT=${STARTING_DIR}/sspl-base/output

pushd sspl-systemtests
sudo -E robot \
    --exclude MANUAL \
    --exclude SLOW \
    --exclude CUSTOM_LOCATION \
    --loglevel TRACE \
    tests
popd
sudo chown -R jenkins: .
