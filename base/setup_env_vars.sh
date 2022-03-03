#!/bin/bash

#source common_vars.sh

# Run this to set environment settings to build SSPL
BUILD_TOOLS_DIR="/build/redist"
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