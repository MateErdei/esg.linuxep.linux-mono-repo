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

[[ -n ${COV_HTML_BASE} ]] || COV_HTML_BASE=sspl-av-unittest
[[ -n ${htmldir} ]] || htmldir=output/coverage/${COV_HTML_BASE}
export htmldir

PRIVATE_KEY=${BASE}/build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || PRIVATE_KEY=build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for upload"

bash ${0%/*}/generateResults.sh || exitFailure $FAILURE_BULLSEYE "Failed to generate bulleye html"

## Ensure ssh won't complain about private key permissions:
chmod 600 ${PRIVATE_KEY}
rsync -va --rsh="ssh -i build/bullseye/private.key" --delete $htmldir \
    upload@allegro.eng.sophos:public_html/bullseye/  \
    </dev/null \
    || exitFailure $FAILURE_BULLSEYE "Failed to upload bulleye html"
