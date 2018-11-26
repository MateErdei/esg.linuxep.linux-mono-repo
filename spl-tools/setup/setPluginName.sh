#!/bin/bash
FAILURE_BAD_ARGUMENT=5
function exitFailure()
{
    local CODE=$1
    shift
    echo "FAILURE - $*" 
    exit $CODE
}

PROG=$0

function usage {
   echo $PROG git_repo --name plugin_name
}

function usageAndExit()
{
    usage
    exitFailure $1 $2
}

PLUGIN_NAME=""
while [[ $# -ge 1 ]]
do
    case $1 in
        --name)
            shift
            PLUGIN_NAME=$1
            PRODUCT=$1
            ;;
        *)
            GITREPO=$1
            #exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done

if [ "x$PLUGIN_NAME" == "x" ]; then
    usageAndExit 3 "Please, provide plugin name"
fi
if [ ! -d $GITREPO ]; then
    usageAndExit 3 "Please, provide the git repository of the fork of template plugin that you want to setup its plugin name"
fi

pushd $GITREPO

echo "create the file mark for the plugin name at PLUGINNAME <- ${PLUGIN_NAME}"
echo -n "$PLUGIN_NAME" > PLUGINNAME
plugin_name="${PLUGIN_NAME,,}"

echo "Setup CMakeLists.txt" 
sed -i "s/sspl_template_plugin/$plugin_name/" CMakeLists.txt

echo "Setup Jenkins file" 
remoteurl=`git config --get remote.origin.url`
sed -i "s/project = 'sspl-template-plugin'/project = '${plugin_name}'/" Jenkinsfile
sed -i "s#project_gitrepo = 'ssh://git@stash.sophos.net:7999/linuxep/sspl-plugin-template.git'#project_gitrepo = '${remoteurl}'#" Jenkinsfile
sed -i "s/project_builddir = 'sspl-template-plugin-build'/project_builddir = '${plugin_name}-plugin-build'/" Jenkinsfile

pushd localjenkins
echo "Setup local jenkins"
sed -i "s/def PLUGIN_NAME = \"sspl-template-plugin\"/def PLUGIN_NAME = \"${PLUGIN_NAME}\"/" Jenkinsfile
popd

echo "Setup build-files" 
pushd build-files
sed -i "s/sspl-template-plugin/${plugin_name}/g" release-package.xml
popd

popd

echo "Now you may proceed to commit and push and setup local jenkins and artisan." 
