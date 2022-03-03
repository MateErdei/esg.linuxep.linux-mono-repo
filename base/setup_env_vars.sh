#!/bin/bash

#source common_vars.sh
export LINUX_ENV_SETUP=true
export ROOT_LEVEL_BUILD_DIR="/build"
export REDIST="$ROOT_LEVEL_BUILD_DIR/redist"
#export BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/build_tools" - atm we can't use a machine setup script so for now we have to work around what's pre-installed to be able to build in CI
export BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/redist"
export FETCHED_INPUTS_DIR="$ROOT_LEVEL_BUILD_DIR/input"
export DEBUG_BUILD_DIR="cmake-build-debug"
export RELEASE_BUILD_DIR="cmake-build-release"



# Run this to set environment settings to build SSPL
export   CC=$BUILD_TOOLS_DIR/gcc/bin/gcc
export  CXX=$BUILD_TOOLS_DIR/gcc/bin/g++
export PATH=$BUILD_TOOLS_DIR/gcc/bin:$PATH
export PATH=$BUILD_TOOLS_DIR/cmake/bin:$PATH
# TODO 'make' and 'as' when not installed via pkg manager

export LIBRARY_PATH=$BUILD_TOOLS_DIR/gcc/lib64:${LIBRARY_PATH}:/usr/lib/x86_64-linux-gnu
#export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$BUILD_TOOLS_DIR/gcc/lib64:${LIBRARY_PATH}
export CPLUS_INCLUDE_PATH=$BUILD_TOOLS_DIR/gcc/include/:/usr/include/x86_64-linux-gnu/:${CPLUS_INCLUDE_PATH}
export CPATH=$BUILD_TOOLS_DIR/gcc/include/:${CPATH}

#export ROOT_LEVEL_BUILD_DIR="/build"
#export FETCHED_INPUTS_DIR="$ROOT_LEVEL_BUILD_DIR/input"

# Make it obvious if someone hasn't sourced this file by changing the prompt to include SSPL
export PS1="(SSPL)$PS1"