#!/bin/bash

set -x

function failure()
{
    echo "$@"
    exit 1
}

BASE=.

[[ -d $BASE/.output ]] || failure "Failed to find bazel build"

# We need:
# base-sdds
# AV-SDDS
# VDL
# model-arm
# local rep
# manual install script

# We need /opt/test/inputs/

function copy_or_link()
{
    ln -vf "$1" "$2" && return
    rsync -v "$1" "$2"
}

TEMP_DIR="$1"
[[ -n "$TEMP_DIR" ]] || TEMPDIR="$(mktemp -d)"
TARFILE="${2:-/tmp/sspl-av-arm64.tar.gz}"
export SUPPLEMENT_DIR=${3:-${SUPPLEMENT_DIR:-$TEMP_DIR/test/inputs}}

echo "Temp tree in $TEMP_DIR"
export AV_ROOT="$TEMP_DIR/test/inputs/av"

mkdir -p "$AV_ROOT"

rsync -va $BASE/av/TA "$TEMP_DIR/test/inputs/"
ln -snf TA $TEMP_DIR/test/inputs/test_scripts

copy_or_link $BASE/.output/base/linux_arm64_rel/installer.zip "$AV_ROOT/base_sdds.zip"
copy_or_link $BASE/.output/av/linux_arm64_rel/installer.zip "$AV_ROOT/av_sdds.zip"

bash $BASE/av/TA/manual/createInstallSet.sh
# No longer required
rm -f "$AV_ROOT/av_sdds.zip"
rm -rf "$AV_ROOT/SDDS-COMPONENT"

exec tar czf "$TARFILE" -C "$TEMP_DIR" .
