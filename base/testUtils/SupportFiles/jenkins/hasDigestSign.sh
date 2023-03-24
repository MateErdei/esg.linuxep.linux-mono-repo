#!/bin/bash
if [[ -n ${TAP_VENV} ]]
then
  source ${TAP_VENV}
  echo "Using venv: $TAP_VENV"
elif [[ -f /tmp/venv-for-ci-tools/bin/activate ]]
then
  source /tmp/venv-for-ci-tools/bin/activate
fi
if [[ ! -x $(which digest_sign) ]]
then
    echo PATH=$PATH
    echo TAP_VENV=$TAP_VENV
    exit 1
fi
exit 0
