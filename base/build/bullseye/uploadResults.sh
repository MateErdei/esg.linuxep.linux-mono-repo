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
[[ -n ${COVERAGE_SCRIPT} ]] || COVERAGE_SCRIPT=/opt/test/inputs/bazel_tools/tools/src/bullseye/test_coverage.py
[[ -n ${UPLOAD_PATH} ]] || UPLOAD_PATH=UnifiedPipelines/linuxep/everest-base
[[ -n ${COVERAGE_TYPE} ]] || COVERAGE_TYPE=full
[[ -n ${TEST_COVERAGE_OUTPUT_JSON} ]] || TEST_COVERAGE_OUTPUT_JSON=/opt/test/results/coverage/test_coverage.json

PRIVATE_KEY=/opt/test/inputs/bullseye_files/private.key
[[ -f ${PRIVATE_KEY} ]] || PRIVATE_KEY=${BASE}/build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for upload"

## Ensure ssh won't complain about private key permissions:
chmod 600 ${PRIVATE_KEY}

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

function remount()
{
    echo before:
    cat /proc/mounts
    umount -fl /mnt/pandorum || true
    umount -fl /mnt/pandorum/BullseyeLM || true
    mount /mnt/pandorum/BullseyeLM || true
    mount /mnt/pandorum || true
    echo after:
    cat /proc/mounts
}

if [[ -z ${UPLOAD_ONLY} ]]
then
  function do_covhtml()
  {
      $BULLSEYE_DIR/bin/covhtml \
          --file "$COVFILE"     \
          --srcdir /            \
          --verbose             \
          "$htmldir"            \
          </dev/null
  }

  function list_exclusions()
  {
      echo "Exclusions:"
      $BULLSEYE_DIR/bin/covselect --list --no-banner --file "$COVFILE"
  }

  if ! list_exclusions
  then
      remount
      list_exclusions
  fi

  #Will produce an error if the source directory is not available but the output html is generated anyway
  #When source is not available the output shows down to the function/member function level

  if ! do_covhtml
  then
      remount
      do_covhtml || exitFailure $FAILURE_BULLSEYE "Failed to generate bullseye html"
  fi
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
      || echo "Failed to upload bullseye html to Allegro"

  if [[ ! -f ${TEST_COVERAGE_OUTPUT_JSON} ]]; then
      sudo mkdir -p ${TEST_COVERAGE_OUTPUT_JSON%/*}
      sudo touch ${TEST_COVERAGE_OUTPUT_JSON}
      sudo chmod +w ${TEST_COVERAGE_OUTPUT_JSON}
  fi

  # Add to PATH so Coverage Script can find covxml
  export PATH=$PATH:$BULLSEYE_DIR/bin
  sudo PATH=$PATH python3 -u $COVERAGE_SCRIPT                   \
      "$COVFILE"                                                \
      --output "$TEST_COVERAGE_OUTPUT_JSON"                     \
      --min-function 70                                         \
      --min-condition 90                                        \
      --upload                                                  \
      --upload-job "$UPLOAD_PATH"                               \
      --coverage-type "$COVERAGE_TYPE"                          \
      || echo "Failed to upload coverage results to Redash"
fi
