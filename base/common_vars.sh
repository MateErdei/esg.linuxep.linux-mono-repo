#!/bin/bash

export ROOT_LEVEL_BUILD_DIR="/build"
export REDIST="$ROOT_LEVEL_BUILD_DIR/redist"
#export BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/build_tools" - atm we can't use a machine setup script so for now we have to work around what's pre-installed to be able to build in CI
export BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/redist"
export FETCHED_INPUTS_DIR="$ROOT_LEVEL_BUILD_DIR/input"
export DEBUG_BUILD_DIR="cmake-build-debug"
export RELEASE_BUILD_DIR="cmake-build-release"