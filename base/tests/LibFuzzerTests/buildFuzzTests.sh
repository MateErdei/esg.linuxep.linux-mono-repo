#!/usr/bin/env bash

# usage: ./buildFuzzTargets.sh

source /etc/profile
set -x
#set -o pipefail

FUZZ_TEST_DIR=$(pwd)
FUZZ_TEST_DIR_NAME=$(basename ${FUZZ_TEST_DIR})
PROJECT_ROOT_SOURCE=$(realpath ${FUZZ_TEST_DIR}/../../)
PROJECT=$(basename ${PROJECT_ROOT_SOURCE})
SSPL_TOOLS_DIR=$(realpath ${PROJECT_ROOT_SOURCE}/../)
FUZZ_TESTCASE_ROOT_DIR="${SSPL_TOOLS_DIR}/everest-systemproducttests/SupportFiles/base_data/fuzz/"

CMAKE_BUILD_DIR=cmake-fuzz
CMAKE_BUILD_FULL_PATH="${PROJECT_ROOT_SOURCE}/${CMAKE_BUILD_DIR}"



# check assumptions:
if [[ "LibFuzzerTests" != "${FUZZ_TEST_DIR_NAME}" ]]; then
  echo "Not executed from LibFuzzerTests: ${FUZZ_TEST_DIR_NAME}"; exit 1;
fi


BASE=${PROJECT_ROOT_SOURCE}
REDIST=/build/redist
INPUT=/build/input

$BASE/setup_build_tools.sh
CI=true $BASE/unpack_build_inputs.sh

## These can't be exitFailure since it doesn't exist till the sourcing is done
[[ -f "$BASE"/build/common.sh ]] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build/common.sh

[[ -f "$BASE"/setup_env_vars.sh ]] || { echo "Can't find setup_env_vars.sh" ; exit 11 ; }
source "$BASE"/setup_env_vars.sh

GCC_TARFILE=$(ls $INPUT/gcc-*-linux.tar.gz)
if [[ -d /build/input/gcc && -f $GCC_TARFILE ]]
then
  pushd $REDIST
  tar xzf $GCC_TARFILE
  popd
fi

export LD_LIBRARY_PATH="$REDIST/gcc/lib64/:${LD_LIBRARY_PATH}"
export PATH="$REDIST/gcc/bin:${PATH}"
export LIBRARY_PATH=$REDIST/gcc/lib64/:${LIBRARY_PATH}:/usr/lib/x86_64-linux-gnu
export CPLUS_INCLUDE_PATH=$REDIST/gcc/include/:/usr/include/x86_64-linux-gnu/:${CPLUS_INCLUDE_PATH}

if [[ -f "$INPUT/cmake/bin/cmake" ]]
then
    ln -sf $INPUT/cmake $REDIST/cmake
    CMAKE=${REDIST}/cmake/bin/cmake
else
    echo "WARNING: using system cmake"
    CMAKE=$(which cmake)
fi


# this initial step is necessary for libprotobuf-mutator to have it built and available.
pushd ${PROJECT_ROOT_SOURCE}/thirdparty
if [[ ! -d libprotobuf-mutator ]]; then
    git clone https://github.com/google/libprotobuf-mutator.git
fi
pushd libprotobuf-mutator
  #Todo remove git checkout and uncomment git pull when https://sophos.atlassian.net/browse/LINUXDAR-6728 is completed
  #git pull
  git checkout tags/v1.1
  cp ${FUZZ_TEST_DIR}/setup_protobuf.patch .
  git checkout CMakeLists.txt
  git apply setup_protobuf.patch

  mkdir -p build
  cd build

  ${CMAKE} .. \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_INSTALL_PREFIX=${PROJECT_ROOT_SOURCE}/thirdparty/output \
    -DLIB_PROTO_MUTATOR_TESTING=OFF \
    -DINPUT=/build/redist

  make -j4  || exitFailure 4 "Failed to build libprotobuf-mutator"
  make install || exitFailure  5 "Failed to build libprotobuf-mutator"
popd # libprotobuf-mutator

popd # thirdparty

TARGETS="ManagementAgentApiTest ZMQTests SimpleFunctionTests PluginApiTest WatchdogApiTest"

# build the executables to fuzz
mkdir -p ${CMAKE_BUILD_FULL_PATH} || exitFailure ${FAILURE_BUILD_FUZZ} "Setup build directory"

pushd ${CMAKE_BUILD_FULL_PATH}

${CMAKE} .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DBUILD_FUZZ_TESTS=ON

make ${TARGETS} || exitFailure  3 "Failed to build fuzz targets"
for TARGET in ${TARGETS}; do
ScriptDir=${CMAKE_BUILD_FULL_PATH}/tests/${FUZZ_TEST_DIR_NAME}
ScriptName=runFuzzer${TARGET}.sh
ScriptPath=${ScriptDir}/${ScriptName}
echo "#!/bin/bash
# Run this script from locally.
# You may extend the examples for the start execution in
# ${FUZZ_TESTCASE_ROOT_DIR}/${TARGET}.

mkdir -p queue
# The detect_odr_violation is removed because the protobuf message is defined twice due to the way that it was imported.
ASAN_OPTIONS=detect_odr_violation=0 ./${TARGET} queue ${FUZZ_TESTCASE_ROOT_DIR}/${TARGET}

# If you wish to run a single target file:
# ASAN_OPTIONS=detect_odr_violation=0 ./${TARGET} <target_file_name>


" > ${ScriptPath}
chmod +x  ${ScriptPath}
echo "${TARGET} produced. You may run it on: ${ScriptDir}
${ScriptName}"
done;

popd
