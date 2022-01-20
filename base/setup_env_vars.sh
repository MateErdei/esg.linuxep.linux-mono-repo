#!/bin/bash

# Run this to set environment settings to build SSPL
# TODO - could not get this working: Add this to your IDE as it's environment file, e.g.: https://www.jetbrains.com/help/clion/how-to-create-toolchain-in-clion.html#env-scripts
echo "test123"


export CC=/home/alex/gitrepos/everest-base/build_tools/gcc/bin/gcc
export CXX=/home/alex/gitrepos/everest-base/build_tools/gcc/bin/g++
export PATH=/home/alex/gitrepos/everest-base/build_tools/gcc/bin:$PATH
export PATH=/home/alex/gitrepos/everest-base/build_tools/cmake/bin:$PATH
# TODO make and as
