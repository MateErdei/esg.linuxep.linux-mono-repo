#!/bin/bash
if [[ -n ${TAP_VENV} ]]
then
  source ${TAP_VENV}
elif [[ -f /tmp/venv-for-ci-tools/bin/activate ]]
then
  source /tmp/venv-for-ci-tools/bin/activate
fi
$@