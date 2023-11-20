#!/usr/bin/env bash

#set -x
set -e

if [[ $(id -u) == 0 ]]
then
    echo "Don't need to run this as root."
    exit 1
fi

CLEAN=0
while [[ $# -ge 1 ]]
do
    case $1 in
        --clean)
            CLEAN=1
            ;;
        *)
            exitFailure $FAILURE_BAD_ARGUMENT "unknown argument $1"
            ;;
    esac
    shift
done


BASEDIR=$(dirname "$0")
# Just in case this script ever gets symlinked
BASEDIR=$(readlink -f "$BASEDIR")

if [[ "$CLEAN" == "1" ]]
then
    source $BASEDIR/setup_env_vars.sh
    echo "Removing $REDIST and $FETCHED_INPUTS_DIR"
    sleep 3
    rm -rf "$REDIST"
    rm -rf "$FETCHED_INPUTS_DIR"
fi

TAP_VENV_DIR="$BASEDIR/../tapvenv"
source "$TAP_VENV_DIR/bin/activate"
export TAP_JWT=$(cat "$BASEDIR/testUtils/SupportFiles/jenkins/jwt_token.txt")
tap fetch linux_mono_repo.products.cmake.base_release || {
  # This is a work around because tap fetch seems to also try and do some sort
  # of build promotion which fails when using jwt_token.txt, so if this is used in a script we need
  # to run the below directly instead of running tap fetch.
  python3 -m build_scripts.artisan_fetch "$BASEDIR/build/release-package.xml"
}
deactivate

if [[ "$CLEAN" == "1" ]]
then
    bold=$(tput bold)
    echo -e "\n${bold}*** You will need to re-run setup_build_tools.sh because the build tools have been removed. ***"
fi
