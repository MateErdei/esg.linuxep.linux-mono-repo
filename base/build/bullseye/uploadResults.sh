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

[[ -n $BULLSEYE_DIR ]] || exitFailure 1 "BULLSEYE_DIR not set"
[[ -n $COVFILE ]] || exitFailure 2 "COVFILE not set"

htmldir=output/coverage/sspl

$BULLSEYE_DIR/bin/covhtml -f "$COVFILE" "$htmldir" \
    </dev/null \
    || exitFailure $FAILURE_BULLSEYE "Failed to generate bulleye html"

PRIVATE_KEY=$BASE/build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for upload"

rsync -va --rsh="ssh -i build/bullseye/private.key" --delete $htmldir \
    upload@allegro.eng.sophos:public_html/bullseye/  \
    </dev/null \
    || exitFailure $FAILURE_BULLSEYE "Failed to upload bulleye html"
