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

source "$BASEDIR"/tap_venv/bin/activate
tap fetch sspl_base.build.release
deactivate

if [[ "$CLEAN" == "1" ]]
then
    bold=$(tput bold)
    echo -e "\n${bold}*** You will need to re-run setup_build_tools.sh because the build tools have been removed. ***"
fi
