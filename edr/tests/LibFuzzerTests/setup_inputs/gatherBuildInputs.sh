#!/usr/bin/env bash
set -x
python3 -m venv /tmp/venv-for-ci
source /tmp/venv-for-ci/bin/activate
  $WORKSPACE/tests/LibFuzzerTests/setup_inputs/SetupCIBuildScripts.sh
  export BUILD_JWT=$(cat $WORKSPACE/tests/LibFuzzerTests/setup_inputs/jwt_token.txt)
  #temp fix for detached head
  git checkout bugfix/LINUXDAR-2616-fix-fuzzing-jobs
  python3 -m build_scripts.artisan_fetch $WORKSPACE/build-files/release-package.xml
deactivate