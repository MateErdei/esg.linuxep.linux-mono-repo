#!/usr/bin/env bash
unset LD_LIBRARY_PATH

SCRIPT_DIR=$(cd "${0%/*}"; echo "$PWD")

function exitFailure()
{
    local E=$1
    shift
    echo "$@"
    exit $E
}

SRC_FILE=$1
[[ -f "$SRC_FILE" ]] || {
    echo  "ERROR: Unable to find src file $SRC_FILE for upload: $?" >&2
    ## don't fail the build for this
    exit 0
}

if [[ -n "$2" ]]
then
    DEST_FILE="$2"
else
    DEST_FILE=$(basename $SRC_FILE)
fi

PRIVATE_KEY=${SCRIPT_DIR}/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for upload"

## Ensure ssh won't complain about private key permissions:
chmod 600 ${PRIVATE_KEY}
rsync -az --rsh="ssh -i ${PRIVATE_KEY} -o StrictHostKeyChecking=no" \
    "$SRC_FILE" \
    "upload@presto.eng.sophos:public_html/robot/$DEST_FILE"  \
    </dev/null \
    || exitFailure 4 "Failed to upload robot log: $?"
