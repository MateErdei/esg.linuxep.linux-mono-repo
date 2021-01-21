#!/usr/bin/env bash
python3 -m venv /tmp/venv-for-ci
source /tmp/venv-for-ci/bin/activate
  $WORKSPACE/testUtils/SupportFiles/jenkins/SetupCIBuildScripts.sh
  export BUILD_JWT=$(cat $WORKSPACE/testUtils/SupportFiles/jenkins/jwt_token.txt)
  python3 -m build_scripts.artisan_fetch $WORKSPACE/build/release-package.xml
deactivate