python3 -m venv /tmp/venv-for-ci
source /tmp/venv-for-ci/bin/activate
  $WORKSPACE/testUtils/SupportFiles/jenkins/SetupCIBuildScripts.sh || fail "Error: Failed to get CI scripts"
  export BUILD_JWT=$(cat $WORKSPACE/testUtils/SupportFiles/jenkins/jwt_token.txt)
  python3 -m build_scripts.artisan_fetch $WORKSPACE/build/release-package.xml || fail "Error: Failed to fetch inputs"
deactivate