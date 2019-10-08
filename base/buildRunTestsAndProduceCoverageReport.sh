#!/bin/bash
set -ex

# assumes this is executed from everest-base/
BASE=$(pwd)
SYSTEM_TEST=""
while [[ $# -ge 1 ]]
do
    case $1 in
        --system-test)
          shift
          SYSTEM_TEST="$1"
          ;;
    esac
    shift
done
echo "build Run Tests and Produce Coverge Report.sh with systemtests: ${SYSTEM_TEST}"
cp /mnt/filer6/linux/SSPL/users/Gesner/coverage.xml "${SYSTEM_TEST}/coverage.xml"
echo "done"
#python3 -m build_scripts.artisan_fetch build/release-package.xml
#./build.sh --python-coverage
#echo "Keep the coverage for unit tests"
#cp modules/.coverage  unit_tests_coverage
#pushd ${SYSTEM_TEST}
#RERUNFAILED=true BASE_SOURCE="${BASE}" bash SupportFiles/jenkins/jenkinsBuildCommand.sh -i SMOKE
#popd
