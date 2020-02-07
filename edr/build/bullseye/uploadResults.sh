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

[[ -n ${BULLSEYE_DIR} ]] || exitFailure 1 "BULLSEYE_DIR not set"
[[ -n ${COVFILE} ]] || exitFailure 2 "COVFILE not set"
[[ -n ${COV_HTML_BASE} ]] || COV_HTML_BASE=sspl-plugin-edr-combined
[[ -n ${htmldir} ]] || htmldir=/opt/test/logs/coverage/${COV_HTML_BASE}

PRIVATE_KEY=${BASE}/build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || PRIVATE_KEY=build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for upload"


if [[ -z ${UPLOAD_ONLY} ]]
then
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
      || exitFailure $FAILURE_BULLSEYE "Failed to generate bulleye html"
fi

chmod -R a+rX "$htmldir"

if [[ ${BULLSEYE_UPLOAD} == 1 ]]
then
  ## Ensure ssh won't complain about private key permissions:
  chmod 600 ${PRIVATE_KEY}
  rsync -va --rsh="ssh -i build/bullseye/private.key" --delete $htmldir \
      upload@allegro.eng.sophos:public_html/bullseye/  \
      </dev/null \
      || exitFailure $FAILURE_BULLSEYE "Failed to upload bulleye html"
fi