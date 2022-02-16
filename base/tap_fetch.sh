#!/usr/bin/env bash

#set -x
set -e

if [[ $(id -u) == 0 ]]
then
    echo "Don't need to run this as root."
    exit 1
fi

BASEDIR=$(dirname "$0")

# Just in case this script ever gets symlinked
BASEDIR=$(readlink -f "$BASEDIR")
source "$BASEDIR"/tap_venv/bin/activate
tap fetch sspl_base.build.release
deactivate
