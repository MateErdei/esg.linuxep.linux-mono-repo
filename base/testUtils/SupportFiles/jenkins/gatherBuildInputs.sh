#!/usr/bin/env bash

PYTHONCOMMAND=python3
if [[ -d /home/jenkins ]]
then
  PYTHON38="$(which python3.8)"
  [[ -x "${PYTHON38}" ]] && PYTHONCOMMAND=python3.8
fi
$PYTHONCOMMAND -m venv /tmp/venv-for-ci
source /tmp/venv-for-ci/bin/activate
  $WORKSPACE/base/testUtils/SupportFiles/jenkins/SetupCIBuildScripts.sh
  export BUILD_JWT=$(cat $WORKSPACE/base/testUtils/SupportFiles/jenkins/jwt_token.txt)
  $PYTHONCOMMAND -m build_scripts.artisan_fetch $WORKSPACE/base/build/release-package.xml
deactivate