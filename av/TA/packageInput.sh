#!/bin/bash
set -ex

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)

OUTPUT=$1
if [[ -z $OUTPUT ]]
then
    OUTPUT=$BASE/../output
fi

INPUTS=/tmp/inputs
AV=$INPUTS/av
mkdir -p $AV

rsync -va "$BASE/../TA/" $INPUTS/TA
rsync -va "$OUTPUT/SDDS-COMPONENT/" "$AV/SDDS-COMPONENT"
rsync -va "$OUTPUT/base-sdds/"      "$AV/base-sdds"
rsync -va "$OUTPUT/test-resources"  "$AV/test-resources"
exec tar cjf /tmp/inputs.tar.bz2 -C /tmp inputs
