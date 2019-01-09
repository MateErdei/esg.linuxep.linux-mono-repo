#!/usr/bin/env bash

FAILURE_BAD_ARG=2
FAILURE_MISSING_ARG=3
FAILURE_MISSING_BASE=4
FAILURE_SETUP_PLUGIN=5

# Ensure we run script from within sspl-tools directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
pushd "${SCRIPT_DIR}/../" &> /dev/null

PROG=$0

function exitFailure()
{
    local CODE=$1
    shift
    echo "FAILURE - $*"
    exit $CODE
}

function usage
{
   echo "Usage: $PROG --name <plugin_name> --project <project_name> [--gitrepo <git_repo>]"
}

function usageAndExit()
{
    usage
    exitFailure $1 $2
}

PLUGIN_NAME=""
PROJECT_NAME=""
while [[ $# -ge 1 ]]
do
    case $1 in
        --name)
            shift
            PLUGIN_NAME=$1
            ;;
        -h | --h | --help)
            usage
            exit 0
            ;;
        --project)
            shift
            PROJECT_NAME=$1
            ;;
        --gitrepo)
            shift
            GITREPO=$1
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARG} "Unknown Argument: $1"
            ;;
    esac
    shift
done

if [ "x$PLUGIN_NAME" == "x" ]; then
    usageAndExit ${FAILURE_MISSING_ARG} "Please provide plugin name. E.g. Example"
fi

if [ "x$PROJECT_NAME" == "x" ]; then
    usageAndExit ${FAILURE_MISSING_ARG} "Please provide project name. E.g. sspl-exampleplugin"
fi

if [ ! -d "exampleplugin" ]; then
    exitFailure ${FAILURE_MISSING_BASE} "exampleplugin repository not found. Run Setup.sh to install all required repos."
fi

PLUGIN_NAME_LOWER="${PLUGIN_NAME,,}"

mkdir $PLUGIN_NAME_LOWER || exitFailure ${FAILURE_SETUP_PLUGIN} "Could not create $PLUGIN_NAME_LOWER directory"
cp -r exampleplugin/* "$PLUGIN_NAME_LOWER/" || exitFailure ${FAILURE_SETUP_PLUGIN} "Could not copy exampleplugin to $PLUGIN_NAME_LOWER"
pushd "$PLUGIN_NAME_LOWER" || exitFailure ${FAILURE_SETUP_PLUGIN} "Could not change directory to $PLUGIN_NAME_LOWER"

[[ -d cmake-build-debug ]] && rm -rf cmake-build-debug
[[ -d build64 ]] && rm -rf build64
[[ -d output ]] && rm -rf output
[[ -d log ]] && rm -rf log

echo "${PLUGIN_NAME}" > PLUGINNAME

sed -i "s/\(DEFAULT_PRODUCT=\).*/\1${PLUGIN_NAME}/" build.sh

sed -i "s/project(.*)/project(${PROJECT_NAME})/" CMakeLists.txt
sed -i "s/\(set(PLUGIN_NAME\).*)/\1 ${PLUGIN_NAME})/" CMakeLists.txt

sed -i "s/\(project = \).*/\1'${PROJECT_NAME}'/" Jenkinsfile
sed -i "s/\(project_builddir = \).*/\1'${PROJECT_NAME}-build'/" Jenkinsfile
if [[ -n $GITREPO ]]
then
    echo $GITREPO
    sed -i "s#\(project_gitrepo = \).*#\1'${GITREPO}'#" Jenkinsfile
else
    sed -i "s#\(project_gitrepo = \).*#\1 ''#" Jenkinsfile
fi

sed -i "s/\(def PLUGIN_NAME = \).*/\1\"${PLUGIN_NAME}\"/" localjenkins/Jenkinsfile


sed -i "s/sspl-exampleplugin/${PROJECT_NAME}/g" build-files/release-package.xml
sed -i "s/Example/${PLUGIN_NAME}/g" build-files/release-package.xml

popd
popd