#!/bin/bash

# Ensure we run script from within sspl-tools directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
pushd "${SCRIPT_DIR}/../" &> /dev/null


CMAKE_CANDIDATES=("/home/pair/clion/bin/cmake" \
                  "/home/pair/clion/bin/cmake/bin/cmake" \
                  "/home/pair/clion/bin/cmake/linux/bin/cmake" \
                  "/opt/clion-2018.2.3/bin/cmake/linux/bin/cmake"
                  )

for candidate in ${CMAKE_CANDIDATES[@]}
do
    if [[ -x ${candidate} ]] && [[ ! -d ${candidate} ]]
    then
        CMAKE=${candidate}
        break
    fi
done

if [[ ! -x ${CMAKE} ]]
then
    echo "Warning: Could not find cmake executable. Using system cmake. \
Please update this script with the correct cmake location for CLion"
    CMAKE=$(which cmake)
    sleep 5
fi

# Do not run as root - we do not want the builds to be root owned
[[ $EUID -ne 0 ]] || error "Please do not run the script as root"

function error()
{
    sleep 0.5
    printf '\033[0;31m'
    echo "ERROR: $1"
    printf '\033[0m'
}

function success()
{
    sleep 0.5
    printf '\033[0;32m'
    echo "$1"
    printf '\033[0m'
}

function prefixwith() {
    local prefix="$1"
    shift
    # Redirect stdout to sed command which appends prefix (and adds colour)
    "$@" > >(sed "s_^_\\o033[32mBuilding $prefix:\\o033[0m _")  2> >(sed "s_^_\\o033[32mBuilding $prefix:\\o033[0m _" >&2)
}

function build
{
    repoName = $1
    pushd $repoName &> /dev/null

    if [[ -f build.sh ]]
    then
        chmod +x build.sh
        if prefixwith "$repoName" ./build.sh
		then
            SUCCESS_BUILDS+=($repoName)
		else
		    FAILED_BUILDS+=($repoName)
        fi
    fi
    popd &> /dev/null
	echo ""
}

function clion-debug-build
{
  project_name=$1
  [[ -f "${project_name}/CMakeLists.txt" ]] || return
  pushd ${project_name}
  SOURCEDIR=`pwd`
  mkdir -p cmake-build-debug
  pushd cmake-build-debug

  FAILED=false
  prefixwith "$project_name" ${CMAKE} -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" ${SOURCEDIR} || FAILED=true
  prefixwith "$project_name" make install dist || FAILED=true

  if [[ "$FAILED" = true ]]
  then
    FAILED_BUILDS+=($project_name)
  else
    SUCCESS_BUILDS+=($project_name)
  fi
  popd
  popd
}

while [[ $# -ge 1 ]]
do
    case $1 in
        --debug)
            DEBUG_BUILD=1
            ;;
    esac
    shift
done

FAILED_BUILDS=()
SUCCESS_BUILDS=()

for repo in $(cat setup/gitRepos.txt)
do
    repoName=$(echo $repo | awk '{n=split($0, a, "/"); print a[n]}' | sed -n "s_\([^\.]*\).*_\1_p")
    if [[ -n $DEBUG_BUILD ]]
    then
        [[ $repoName != *"thininstaller"* ]] && clion-debug-build $repoName
    else
        build $repoName
    fi
done
echo ""

for build in ${SUCCESS_BUILDS[@]}
do
    success "Build of ${build} succeeded!"
done

for build in ${FAILED_BUILDS[@]}
do
    error "Build of ${build} failed!"
done

popd &> /dev/null
