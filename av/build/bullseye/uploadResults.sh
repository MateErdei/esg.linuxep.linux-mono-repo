#!/usr/bin/env bash
echo "Uploading bullseye output"
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
export COV_HTML_BASE
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
rsync -az --rsh="ssh -i ${PRIVATE_KEY}  -o StrictHostKeyChecking=no" --delete ${htmldir}/ \
    upload@allegro.eng.sophos:public_html/bullseye/${COV_HTML_BASE}/  \
    </dev/null \
    || exitFailure $FAILURE_BULLSEYE "Failed to upload bulleye html"
