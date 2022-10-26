#!/bin/bash

export ROOT_LEVEL_BUILD_DIR="/build"
export REDIST="$ROOT_LEVEL_BUILD_DIR/redist"
#export BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/build_tools" - atm we can't use a machine setup script so for now we have to work around what's pre-installed to be able to build in CI
export BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/redist"
export FETCHED_INPUTS_DIR="$ROOT_LEVEL_BUILD_DIR/input"
export DEBUG_BUILD_DIR="cmake-build-debug"
export RELEASE_BUILD_DIR="cmake-build-release"

export   CC=$BUILD_TOOLS_DIR/gcc/bin/gcc
export  CXX=$BUILD_TOOLS_DIR/gcc/bin/g++
export PATH=$BUILD_TOOLS_DIR/gcc/bin:$PATH
export PATH=$BUILD_TOOLS_DIR/cmake/bin:$PATH
# TODO 'make' and 'as' when not installed via pkg manager

# Cmake can use different generators, modern CLion uses ninja and not makefiles. By default CMake uses
# unix makefiles. This means there is a clash when generating the build files, CLion generates ninja build files
# and build.sh will generate unix makefiles, so for now we'll just force everything to use makefiles.
export CMAKE_GENERATOR="Unix Makefiles"

export LIBRARY_PATH=$BUILD_TOOLS_DIR/gcc/lib64:${LIBRARY_PATH}:/usr/lib/x86_64-linux-gnu
export LD_LIBRARY_PATH="${LIBRARY_PATH}"
export CPLUS_INCLUDE_PATH=$BUILD_TOOLS_DIR/gcc/include/:/usr/include/x86_64-linux-gnu/:${CPLUS_INCLUDE_PATH}
export CPATH=$BUILD_TOOLS_DIR/gcc/include/:${CPATH}

# For those using cmdline only, make it obvious if someone hasn't sourced this file by changing the prompt
export PS1="(SSPL)$PS1"

export LINUX_ENV_SETUP="true"