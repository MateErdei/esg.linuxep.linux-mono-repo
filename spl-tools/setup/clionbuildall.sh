#!/bin/bash
CLION_ROOT=/home/pair/clion
CMAKE=${CLION_ROOT}/bin/cmake/linux/bin/cmake


function clion-debug-build
{
  project_name=$1
  pushd ${project_name}
  SOURCEDIR=`pwd`
  mkdir -p cmake-build-debug
  pushd cmake-build-debug
  ${CMAKE} -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" ${SOURCEDIR}
  make install dist
  popd
  popd
  
}

if [ ! -d everest-base ]; then
  echo 'No everest-base in the current folder'
  exit 1
fi

clion-debug-build everest-base
clion-debug-build exampleplugin



