#!/usr/bin/env bash
python3 -m venv /tmp/venv-for-ci
source /tmp/venv-for-ci/bin/activate
  $WORKSPACE/tests/LibFuzzerTests/SetupCIBuildScripts.sh
  export BUILD_JWT=$(cat $WORKSPACE/tests/LibFuzzerTests/jwt_token.txt)
  python3 -m build_scripts.artisan_fetch $WORKSPACE/build-files/release-package.xml
deactivate