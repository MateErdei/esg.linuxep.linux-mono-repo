#!/usr/bin/env bash
echo "Process bullseye output"
unset LD_LIBRARY_PATH

FAILURE_BULLSEYE=52

function exitFailure()
{
    local E=$1
    shift
    echo "$@"
    exit $E
}

htmldir=output/coverage/sspl

$BULLSEYE_DIR/bin/covhtml -f "$COVFILE" "$htmldir" || exitFailure $FAILURE_BULLSEYE "Failed to generate bulleye html"

rsync -va --rsh="ssh -i build/bullseye/private.key" --delete $htmldir \
    upload@allegro.eng.sophos:public_html/bullseye/ \
    || exitFailure $FAILURE_BULLSEYE "Failed to upload bulleye html"
