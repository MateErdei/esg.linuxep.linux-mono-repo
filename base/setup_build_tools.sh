#!/bin/bash


#set -x
set -e

## This had to be commented out for Continous Fuzzer fix. Still desirable code if we can improve the Fuzzer impl.
#if [[ $(id -u) == 0 ]]
#then
#    echo "You don't need to run the entire script as root, sudo will prompt you if needed."
#    exit 1
#fi

BASEDIR=$(dirname "$0")
# Just in case this script ever gets symlinked
BASEDIR=$(readlink -f "$BASEDIR")

CURRENT_USER=$USER
[[ -n "$CURRENT_USER" ]] || CURRENT_USER=$(whoami)

if [[ "$LINUX_ENV_SETUP" != "true" ]]
then
  source "$BASEDIR/setup_env_vars.sh"
fi

function install_package()
{
  local package=$1
  if which apt &>/dev/null
  then
    if ! apt list --installed $package 2>/dev/null | grep -q "$package"
    then
      sudo apt-get install $package -y || exit 1
    fi
  else
    # TODO support yum
    echo "Not implemented yet - currently only support apt"
  fi
}


cd "$BASEDIR"

# Where the TAP Python venv will be put
TAP_VENV="$BASEDIR/../tapvenv"

# Install python3 venv for TAP and then install TAP
# For TAP we try to work out which python to use or install and then set PYTHON_TO_USE for use by TAP and TAP venv.
if which python3
then
  PYTHON_TO_USE=python3
else
  echo "Python3 not installed"
  exit 1
fi

if python3 -c "import sys; print(sys.version_info.major >= 3 and sys.version_info.minor >= 7)" | grep False
then
  echo "python3 version is too old, trying 'python3.8' explicitly"
  if which python3.8
  then
    PYTHON_TO_USE=python3.8
  else
    echo "Python3.8 not installed, trying python3.7"
    if which python3.7
      then
        PYTHON_TO_USE=python3.7
      else
        echo "Python3.7 not installed, will try installing python3.8"
        install_package  python3.8-venv
        if which python3.8
        then
          PYTHON_TO_USE=python3.8
        else
          echo "Could not install python3.8, will try installing python3.7"
          install_package  python3.7-venv
          if which python3.7
          then
            PYTHON_TO_USE=python3.7
          else
            echo "Could not find or install a python version of 3.7 or newer."
            exit 1
          fi
        fi
      fi
  fi
fi
echo "Using \"$PYTHON_TO_USE\" for python commands"

install_package $PYTHON_TO_USE-venv

if [ -d "$TAP_VENV" ]
then
    echo "$TAP_VENV already exists, not re-creating it"
    source "$TAP_VENV/bin/activate"
    echo "Updating pip and tap:"
    python3 -m pip install pip --upgrade
    python3 -m pip install tap --upgrade
else
    $PYTHON_TO_USE -m venv "$TAP_VENV"
    pushd "$TAP_VENV"
        echo "[global]" > pip.conf
        echo "index-url = https://artifactory.sophos-ops.com/api/pypi/pypi/simple" >> pip.conf
        echo "trusted-host = artifactory.sophos-ops.com" >> pip.conf
    popd

    # Within TAP python venv just use "python3"
    source "$TAP_VENV/bin/activate"
        echo "Using python:"
        which python
        python3 -m pip install pip --upgrade
        python3 -m pip install wheel
        ##python3 -m pip install build_scripts
        python3 -m pip install keyrings.alt
        python3 -m pip install tap
fi

# TODO - raise ticket to get this dependency removed - Not sure why tap needs this, seems to never be used and is always empty.
TAP_CACHE="/SophosPackages"
if [ -d "$TAP_CACHE" ]
then
    echo "TAP cache dir already exists: $TAP_CACHE"
else
    echo "Creating TAP cache dir: $TAP_CACHE"
    sudo mkdir "$TAP_CACHE"
    sudo chown "$CURRENT_USER" "$TAP_CACHE"
fi

# Temporary - When in monorepo we will not need to do this and have it in / and created by root, we'll be able to have
# it in the root of the repo dir or similar
#ROOT_LEVEL_BUILD_DIR="/build"
if [ -d "$ROOT_LEVEL_BUILD_DIR" ]
then
    echo "Root level dir already exists: $ROOT_LEVEL_BUILD_DIR"
else
    echo "Creating Root level dir: $ROOT_LEVEL_BUILD_DIR"
    sudo mkdir "$ROOT_LEVEL_BUILD_DIR"
    sudo chown "$CURRENT_USER" "$ROOT_LEVEL_BUILD_DIR"
fi

tap --version
tap ls
export TAP_JWT=$(cat "$BASEDIR/testUtils/SupportFiles/jenkins/jwt_token.txt")
tap fetch linux_mono_repo.products.cmake.base_release || {
  # This is a work around because tap fetch seems to also try and do some sort
  # of build promotion which fails when using jwt_token.txt, so if this is used in a script we need
  # to run the below directly instead of running tap fetch.
  python3 -m build_scripts.artisan_fetch "$BASEDIR/build/release-package.xml"
}
# We're done with TAP now, so deactivate the venv.
deactivate

#BUILD_TOOLS_DIR="$ROOT_LEVEL_BUILD_DIR/build_tools"
#FETCHED_INPUTS_DIR="$ROOT_LEVEL_BUILD_DIR/input"
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
CMAKE_DIR="$BUILD_TOOLS_DIR/cmake"
if [[ -f "$FETCHED_INPUTS_DIR/cmake/bin/cmake" ]]
then
    if [[ ! -d $REDIST/cmake ]]
    then
        ln -sf $FETCHED_INPUTS_DIR/cmake $REDIST/cmake
    fi
else
    echo "ERROR - cmake not found here: $FETCHED_INPUTS_DIR/cmake/bin/cmake"
    exit 1
fi
chmod 700 "$CMAKE_DIR/bin/cmake" || exitFailure "Unable to chmod cmake"
chmod 700 "$CMAKE_DIR/bin/ctest" || exitFailure "Unable to chmod ctest"
echo "---"
echo "cmake synced to $REDIST/cmake"
"$CMAKE_DIR/bin/cmake" --version


# Make
# TODO
# Make is not yet available in artifactory so for now we'll install it directly
install_package make
MAKE_DIR="$BUILD_TOOLS_DIR/make"
mkdir -p "$MAKE_DIR"
which make &> /dev/null
pushd "$MAKE_DIR"
  ln -fs "$(which make)" "make"
popd
echo "---"
"$MAKE_DIR/make" --version


# AS (assembler)
# TODO
# AS (assembler) is not yet available in artifactory so for now we'll install it directly
install_package binutils
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
install_package libc6-dev

# zip is used when packing up mcs router
install_package zip

# When we use WSL2 and run CLion directly on windows the debugger in CLion cannot be used so we have to install GDB
install_package gdb

# We are temporarily using chrpath until we properly fix RPATH for pluginanpi and thirdparties.
install_package chrpath

# We use "proxy" in tests/Common/HttpRequesterImpl/HttpTestServer.py which is used by
# tests/Common/HttpRequesterImpl/HttpRequesterLiveNetworkTests.cpp
install_package libproxy-tools

# AV plugin relies on system zlib, need to install it system-wide
install_package zlib1g-dev

# IFilePermissions.h in /modules/Common/FileSystem/ includes <sys/capability.h>, header file comes from libcap-dev
install_package libcap-dev

# We depend on the TAP script ./tap_venv/bin/sb_manifest_sign
# To make this easy to find we add a symlink that will be found on the default PATH.
if [[ -f "$TAP_VENV/bin/sb_manifest_sign" ]]
then
  if which sb_manifest_sign
  then
    echo "Skipping adding symlink for sb_manifest_sign"
  else
    sudo ln -s "$TAP_VENV/bin/sb_manifest_sign" "/usr/local/bin/sb_manifest_sign"
  fi
else
  echo "Could not find $TAP_VENV/bin/sb_manifest_sign"
  exit 1
fi

WAREHOUSE_DIR=$(realpath "$BASEDIR/../esg.linuxep.sspl-warehouse")
if [[ -d "$WAREHOUSE_DIR" ]]
then
  echo "Linking base testUtils to warehouse repo"
  ln -sf $BASEDIR/testUtils/libs $WAREHOUSE_DIR/TA/libs
  ln -sf $BASEDIR/testUtils/SupportFiles $WAREHOUSE_DIR/TA/SupportFiles
fi

echo "---"
echo "Done"
echo "To build, you can use one of these options:"
echo "a) In your IDE set the toolchain environment file to be: 'setup_env_vars.sh' and then select build."
echo "   > File | Settings | Build, Execution, Deployment | Toolchains"
echo "b) run ./build.sh"
echo "c) cmdline 'source setup_env_vars.sh' and then: "
echo "    mkdir cmake-build-debug && cd cmake-build-debug"
echo "    cmake .. && make install"
