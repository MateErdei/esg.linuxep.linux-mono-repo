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

[[ -n ${BULLSEYE_DIR} ]] || exitFailure 1 "BULLSEYE_DIR not set"
[[ -n ${COVFILE} ]] || exitFailure 2 "COVFILE not set"
[[ -n ${COV_HTML_BASE} ]] || COV_HTML_BASE=sspl-av-unittest
[[ -n ${htmldir} ]] || htmldir=output/coverage/${COV_HTML_BASE}
[[ -n ${srcdir} ]] || scrdir=$PWD

echo "Exclusions:"
$BULLSEYE_DIR/bin/covselect --list --no-banner --file "$COVFILE"

$BULLSEYE_DIR/bin/covhtml \
    --file "$COVFILE"     \
    --srcdir "$srcdir"    \
    --verbose             \
    "$htmldir"            \
    </dev/null            \
    || exitFailure $FAILURE_BULLSEYE "Failed to generate bulleye html"

chmod -R a+rX "$htmldir"
