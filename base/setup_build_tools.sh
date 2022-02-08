#!/bin/bash

#set -x
set -e

if [[ $(id -u) == 0 ]]
then
    echo "You don't need to run the entire script as root, sudo will prompt you if needed."
    exit 1
fi

BASEDIR=$(dirname "$0")

# Just in case this script ever gets symlinked
BASEDIR=$(readlink -f "$BASEDIR")
cd "$BASEDIR"

# Where the TAP Python venv will be put
TAP_VENV="$BASEDIR/tap_venv"

# Install python3 venv for TAP and then install TAP
# TODO deal with OSs with python3.6 as deafult = e.g. 18.04
if ! apt list --installed python3-venv 2>/dev/null | grep -q python3-venv
then
  sudo apt-get install python3-venv -y
fi

if [ -d "$TAP_VENV" ]
then
 echo "$TAP_VENV already exists, not re-creating it"
 source "$TAP_VENV/bin/activate"
else
  python3 -m venv "$TAP_VENV"
  pushd "$TAP_VENV"
    echo "[global]" > pip.conf
    echo "index-url = https://tap-artifactory1.eng.sophos/artifactory/api/pypi/pypi/simple" >> pip.conf
    echo "trusted-host = tap-artifactory1.eng.sophos" >> pip.conf
  popd
  source "$TAP_VENV/bin/activate"
  echo "Using python:"
  which python
  python3 -m pip install pip --upgrade
  python3 -m pip install wheel
  ##python3 -m pip install build_scripts
  python3 -m pip install keyrings.alt
  python3 -m pip install tap
fi

# TODO - raise ticket to get this dependency removed - Not sure why tap needs this, seems to never be used?
TAP_CACHE="/SophosPackages"
if [ -d "$TAP_CACHE" ]
then
  echo "TAP cache dir already exists: $TAP_CACHE"
else
  echo "Creating TAP cache dir: $TAP_CACHE"
  sudo mkdir "$TAP_CACHE"
  sudo chown "$USER" "$TAP_CACHE"
fi

# Temporary - When in monorepo we will not need to do this and have it in / and created by root, we'll be able to have
# it in the root of the repo dir or similar
ROOT_LEVEL_BUILD_DIR="/build"
if [ -d "$ROOT_LEVEL_BUILD_DIR" ]
then
  echo "Root level dir already exists: $ROOT_LEVEL_BUILD_DIR"
else
  echo "Creating Root level dir: $ROOT_LEVEL_BUILD_DIR"
  sudo mkdir "$ROOT_LEVEL_BUILD_DIR"
  sudo chown "$USER" "$ROOT_LEVEL_BUILD_DIR"
fi

tap --version
tap ls
tap fetch sspl_base.build.release

BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/build_tools"
FETCHED_INPUTS_DIR="$ROOT_LEVEL_BUILD_DIR/input"
if [[ ! -d "$FETCHED_INPUTS_DIR" ]]
then
  echo "$FETCHED_INPUTS_DIR does not exist"
  exit 1
fi

[[ -d "$BUILD_TOOLS_DIR" ]] || mkdir -p "$BUILD_TOOLS_DIR"

# GCC
# input/gcc-8.1.0-linux.tar.gz
GCC_TARFILE=$(ls $FETCHED_INPUTS_DIR/gcc-*.tar.gz)
GCC_TARFILE_HASH=$(md5sum "$GCC_TARFILE" | cut -d ' ' -f 1)
GCC_DIR="$BUILD_TOOLS_DIR/gcc"

function unpack_gcc()
{
  mkdir "$GCC_DIR"
#  tar -xzf "$GCC_TARFILE" -C "$GCC_DIR"
  tar -xzf "$GCC_TARFILE" -C "$BUILD_TOOLS_DIR"
  touch "$GCC_DIR/$GCC_TARFILE_HASH"
}

if [[ -d "$GCC_DIR" ]]
then
  if [[ -f "$GCC_DIR/$GCC_TARFILE_HASH" ]]
  then
    echo "Already unpacked GCC"
  else
    echo "Removing old GCC build tools"
    rm -rf "$GCC_DIR"
    unpack_gcc
  fi
else
   unpack_gcc
fi
echo "---"
"$GCC_DIR/bin/gcc" --version
echo "---"
"$GCC_DIR/bin/g++" --version


# CMAKE
# input/cmake-3.11.2-linux.tar.gz
CMAKE_TARFILE=$(ls $FETCHED_INPUTS_DIR/cmake-*.tar.gz)
CMAKE_TARFILE_HASH=$(md5sum "$CMAKE_TARFILE" | cut -d ' ' -f 1)
CMAKE_DIR="$BUILD_TOOLS_DIR/cmake"

function unpack_cmake()
{
  mkdir "$CMAKE_DIR"
#  tar -xzf "$CMAKE_TARFILE" -C "$CMAKE_DIR"
  tar -xzf "$CMAKE_TARFILE" -C "$BUILD_TOOLS_DIR"
  touch "$CMAKE_DIR/$CMAKE_TARFILE_HASH"
}

if [[ -d "$CMAKE_DIR" ]]
then
  if [[ -f "$CMAKE_DIR/$CMAKE_TARFILE_HASH" ]]
  then
    echo "Already unpacked cmake"
  else
    echo "Removing old cmake build tools"
    rm -rf "$CMAKE_DIR"
    unpack_cmake
  fi
else
   unpack_cmake
fi
echo "---"
"$CMAKE_DIR/bin/cmake" --version


# Make
# TODO
# Make is not yet available in artifactory so for now we'll install it directly
if ! apt list --installed make 2>/dev/null | grep -q make
then
  sudo apt-get install make -y
fi
MAKE_DIR="$BUILD_TOOLS_DIR/make"
mkdir -p "$MAKE_DIR"
which make
pushd "$MAKE_DIR"
ln -fs "$(which make)" "make"
popd
echo "---"
"$MAKE_DIR/make" --version


# AS (assembler)
# TODO
# AS (assembler) is not yet available in artifactory so for now we'll install it directly
if ! apt list --installed binutils 2>/dev/null | grep -q binutils
then
  sudo apt-get install binutils -y
fi
AS_DIR="$BUILD_TOOLS_DIR/as"
mkdir -p "$AS_DIR"
which as
pushd "$AS_DIR"
ln -fs "$(which as)" "as"
popd
echo "---"
"$AS_DIR/as" --version

# libc6-dev (libc dev packages)
# TODO - try and use crosstoolng to build a GCC toolchain
# This install works around this error:
# /usr/bin/ld: cannot find crt1.o: No such file or directory
if ! apt list --installed libc6-dev 2>/dev/null | grep -q libc6-dev
then
  sudo apt-get install libc6-dev -y
fi

# zip is used when packing up mcs router
if ! apt list --installed zip 2>/dev/null | grep -q zip
then
  sudo apt install zip -y
fi

echo "---"
echo "Done, please 'source setup_env_vars.sh'"

# TODO For SSPL only dev machines this may be ok but might break other things?
# [[ -f /etc/profile.d/setup_env_vars.sh ]] || sudo ln -s "$BASEDIR/setup_env_vars.sh" /etc/profile.d/setup_env_vars.sh
#source ./setup_env_vars.sh?

#echo "Please reboot build machine to apply env changes or source $BASEDIR/setup_env_vars.sh"


#TODO try using build-tools-package.xml only