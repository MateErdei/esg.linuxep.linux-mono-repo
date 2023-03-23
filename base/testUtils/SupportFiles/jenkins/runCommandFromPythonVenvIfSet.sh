#!/bin/bash
if [[ -n ${TAP_VENV} ]]
then
  source ${TAP_VENV}
  echo "Using venv: $TAP_VENV"
fi
exec "$@"