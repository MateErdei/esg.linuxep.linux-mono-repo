#!/usr/bin/env bash

FUZZ_TEST_DIR=$(pwd)
FUZZ_TEST_DIR_NAME=$(basename ${FUZZ_TEST_DIR})
PROJECT_ROOT_SOURCE=$(realpath ${FUZZ_TEST_DIR}/../../)
PROJECT=$(basename ${PROJECT_ROOT_SOURCE})
SSPL_TOOLS_DIR=$(realpath ${PROJECT_ROOT_SOURCE}/../)
FUZZ_TESTCASE_ROOT_DIR="${FUZZ_TEST_DIR}/data/"

CMAKE_BUILD_DIR=cmake-fuzz
CMAKE_BUILD_FULL_PATH="${PROJECT_ROOT_SOURCE}/${CMAKE_BUILD_DIR}"

set -x
# check assumptions:
if [[ "FuzzerTests" != "${FUZZ_TEST_DIR_NAME}" ]]; then
  echo "Not executed from LibFuzzerTests: ${FUZZ_TEST_DIR_NAME}"; exit 1;
fi

BASE=${PROJECT_ROOT_SOURCE}

## These can't be exitFailure since it doesn't exist till the sourcing is done
[[ -f "$BASE"/build-files/common.sh ]] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build-files/common.sh


REDIST=$BASE/redist
INPUT=$BASE/input

CMAKE_TAR=$(ls $INPUT/cmake-*.tar.gz)
if [[ -f "$CMAKE_TAR" ]]
then
    tar xzf "$CMAKE_TAR" -C "$REDIST"
    CMAKE=${REDIST}/cmake/bin/cmake
else
    echo "WARNING: using system cmake"
    CMAKE=$(which cmake)
fi

ZIP=$(which zip)
if [[ ! -x ${ZIP} ]]
then
    apt-get -y install zip unzip
fi


# this initial step is necessary for libprotobuf-mutator to have it built and available.
pushd ${PROJECT_ROOT_SOURCE}/thirdparty
if [[ ! -d libprotobuf-mutator ]]; then
    git clone https://github.com/google/libprotobuf-mutator.git
fi
pushd libprotobuf-mutator
  # at this point in time their project has not release. Hence, just update
  git pull
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
    -DINPUT=${PROJECT_ROOT_SOURCE}/redist
  make -j4
  make install
popd # libprotobuf-mutator

popd # thirdparty

TARGETS="LiveQueryTests LiveQueryInputTests"

# build the executables to fuzz
mkdir -p ${CMAKE_BUILD_FULL_PATH} || exitFailure ${FAILURE_BUILD_FUZZ} "Setup build directory"

pushd ${CMAKE_BUILD_FULL_PATH}

${CMAKE} .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DBUILD_FUZZ_TESTS=ON -DNO_GCOV="true"



make ${TARGETS}
for TARGET in ${TARGETS}; do
ScriptDir=${CMAKE_BUILD_FULL_PATH}/tests/${FUZZ_TEST_DIR_NAME}
ScriptName=runFuzzer${TARGET}.sh
ScriptPath=${ScriptDir}/${ScriptName}
EXTRA_ARGS=""
if [ -f ${FUZZ_TEST_DIR}/${TARGET}.dict ];then
EXTRA_ARGS="-dict=${FUZZ_TEST_DIR}/${TARGET}.dict"
fi
echo "#!/bin/bash
# Run this script from locally.
# You may extend the examples for the start execution in
# ${FUZZ_TESTCASE_ROOT_DIR}/${TARGET}.

mkdir -p queue
# The detect_odr_violation is removed because the protobuf message is defined twice due to the way that it was imported.
ASAN_OPTIONS=detect_odr_violation=0 ./${TARGET} ${EXTRA_ARGS} queue ${FUZZ_TESTCASE_ROOT_DIR}/${TARGET}

# If you wish to run a single target file:
# ASAN_OPTIONS=detect_odr_violation=0 ./${TARGET} <target_file_name>


" > ${ScriptPath}
chmod +x  ${ScriptPath}
echo "${TARGET} produced. You may run it on: ${ScriptDir}
${ScriptName}"
done;

popd
