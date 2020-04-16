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

SCRIPT_DIR=$(cd "${0%/*}"; echo "$PWD")

[[ -n ${COV_HTML_BASE} ]] || COV_HTML_BASE=sspl-av-unittest
[[ -n ${htmldir} ]] || htmldir=output/coverage/${COV_HTML_BASE}
export htmldir

PRIVATE_KEY=${SCRIPT_DIR}/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for upload"

if [[ -z ${UPLOAD_ONLY} ]]
then
  bash ${0%/*}/generateResults.sh || exitFailure $FAILURE_BULLSEYE "Failed to generate bulleye html"
fi

## Ensure ssh won't complain about private key permissions:
chmod 600 ${PRIVATE_KEY}
rsync -va --rsh="ssh -i ${PRIVATE_KEY}" --delete $htmldir \
    upload@allegro.eng.sophos:public_html/bullseye/  \
    </dev/null \
    || exitFailure $FAILURE_BULLSEYE "Failed to upload bulleye html"
