#!/usr/bin/env bash
set -x
python3 -m venv /tmp/venv-for-ci
source /tmp/venv-for-ci/bin/activate
  $WORKSPACE/tests/FuzzerTests/setup_inputs/SetupCIBuildScripts.sh
  export BUILD_JWT=$(cat $WORKSPACE/tests/FuzzerTests/setup_inputs/jwt_token.txt)
  #fix for detached head
  git branch temp
  git checkout temp
  python3 -m build_scripts.artisan_fetch $WORKSPACE/build-files/release-package.xml
deactivate