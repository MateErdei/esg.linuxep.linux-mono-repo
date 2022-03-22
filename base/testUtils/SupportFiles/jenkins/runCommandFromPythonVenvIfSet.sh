#!/bin/bash
if [[ -n ${TAP_VENV} ]]
then
  source ${TAP_VENV}
fi
$@