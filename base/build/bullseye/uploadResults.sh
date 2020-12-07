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
[[ -n ${BASE} ]] || BASE=${SCRIPT_DIR}/../..

[[ -n ${COV_HTML_BASE} ]] || COV_HTML_BASE=sspl-base-unittest
[[ -n ${htmldir} ]] || htmldir=${BASE}/output/coverage/${COV_HTML_BASE}

PRIVATE_KEY=/opt/test/inputs/bullseye_files/private.key
[[ -f ${PRIVATE_KEY} ]] || PRIVATE_KEY=${BASE}/build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for upload"

## Ensure ssh won't complain about private key permissions:
chmod 600 ${PRIVATE_KEY}

if [[ -z ${UPLOAD_ONLY} ]]
then
  # tap template bullseye is installed to /usr/local/bin,
  # jenkins job template installs to either /opt/BullseyeCoverage or /usr/local/bullseye
  if [[ -f /opt/BullseyeCoverage/bin/covselect ]]
  then
    BULLSEYE_DIR=/opt/BullseyeCoverage
  elif [[ -f /usr/local/bin/covselect ]]
  then
    BULLSEYE_DIR=/usr/local
  elif [[ -f /usr/local/bullseye/bin/covselect ]]
  then
      BULLSEYE_DIR=/usr/local/bullseye
  else
    exitFailure $FAILURE_BULLSEYE "Failed to find bulleye"
  fi

  echo "Bullseye location: $BULLSEYE_DIR"

  echo "Exclusions:"
  $BULLSEYE_DIR/bin/covselect --list --no-banner --file "$COVFILE"

  #Will produce an error if the source directory is not available but the output html is generated anyway
  #When source is not avaible the output shows down to the function/member function level
  $BULLSEYE_DIR/bin/covhtml \
      --file "$COVFILE"     \
      --srcdir /            \
      --verbose             \
      "$htmldir"            \
      </dev/null            \
      || exitFailure $FAILURE_BULLSEYE "Failed to generate bullseye html"
else
  #upload results
  BULLSEYE_UPLOAD=1
fi

chmod -R a+rX "$htmldir"

if [[ ${BULLSEYE_UPLOAD} == 1 ]]
then
  rsync -va --rsh="ssh -i ${PRIVATE_KEY} -o StrictHostKeyChecking=no" --delete $htmldir \
      upload@allegro.eng.sophos:public_html/bullseye/  \
      </dev/null \
      || exitFailure $FAILURE_BULLSEYE "Failed to upload bullseye html"
fi