#!/usr/bin/env bash

# usage: ./buildFuzzTargets.sh
# config options: you may want to change the PATH to the afl-latest.tgz

FAILURE_INVALID_AFL_PATH=16
FAILURE_BUILD_AFL=17
FAILURE_BUILD_FUZZ=18
FAILURE_BAD_ARGUMENT=53


STARTINGDIR=$(pwd)

cd ${0%/*}
FuzzTestsDir=$(pwd)
SOURCE_DIR=$(realpath ${FuzzTestsDir}/../../../)
SSPL_TOOLS_DIR=$(realpath ${FuzzTestsDir}/../../../../)
FuzzTestCaseRelDir="tests/eventjournaler/FuzzerTests/AflFuzzScripts/data"

PROJECT=sspl-plugin-event-journaler


# check assumptions:
if [[ "AflFuzzScripts" != "$(basename ${FuzzTestsDir})" ]]; then
  echo "Not executed from FuzzTests: ${FuzzTestsDir}"; exit 1;
fi


CMAKE_BUILD_DIR=cmake-afl-fuzz
CMAKE_BUILD_FULL_PATH="${SOURCE_DIR}/${CMAKE_BUILD_DIR}"

BASE=${SOURCE_DIR}
REDIST=/build/redist
INPUT=/build/input

[[ -f "$BASE"/build-files/common.sh ]] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build-files/common.sh

PATH_TO_AFL_LATEST_TAR_GZ=/mnt/filer6/linux/SSPL/testautomation/afl/afl-latest.tgz
# ensure the afl tools is available in sspl-tools
AFL_BASE_NAME=$(tar tfz "${PATH_TO_AFL_LATEST_TAR_GZ}" --exclude '*/*')

AFL_PATH="${SSPL_TOOLS_DIR}/${AFL_BASE_NAME}"

# decompress the latest afl source to sspl-tools directory
if [[ ! -d ${AFL_PATH} ]]; then
    tar xzvf "${PATH_TO_AFL_LATEST_TAR_GZ}" -C "${SSPL_TOOLS_DIR}"
fi

if [[ ! -f ${AFL_PATH}/afl-gcc.c ]]; then
  exitFailure  ${FAILURE_INVALID_AFL_PATH} "Invalid afl path"
fi

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
export CC=$REDIST/gcc/bin/gcc
export CXX=$REDIST/gcc/bin/g++
if [[ -f "$INPUT/cmake/bin/cmake" ]]
then
    ln -sf $INPUT/cmake $REDIST/cmake
    CMAKE=${REDIST}/cmake/bin/cmake
else
    echo "WARNING: using system cmake"
    CMAKE=$(which cmake)
fi

ZIP=$(which zip)
if [[ ! -x ${ZIP} ]]
then
    apt-get -y install zip
fi

# make sure afl is built
pushd  ${AFL_PATH}
  make clean || exitFailure ${FAILURE_BUILD_AFL} "Failed to build afl"
  make || exitFailure ${FAILURE_BUILD_AFL} "Failed to build afl"
popd


if [[ ! -f ${AFL_PATH}/afl-gcc ]]; then
  exitFailure  ${FAILURE_BUILD_AFL}  "afl-gcc not in the expected path"
fi

TARGETS="eventpubsubtests"

# build the executables to fuzz
mkdir -p ${CMAKE_BUILD_FULL_PATH} || exitFailure ${FAILURE_BUILD_FUZZ} "Setup build directory"

pushd ${CMAKE_BUILD_FULL_PATH}
  ${CMAKE} -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" -DCMAKE_CXX_COMPILER="${AFL_PATH}/afl-g++" -DCMAKE_C_COMPILER="${AFL_PATH}/afl-gcc" -DBUILD_FUZZ_TESTS=True  "${SOURCE_DIR}"
  AFL_HARDEN=1  make -j4 copy_libs ${TARGETS} || exitFailure ${FAILURE_BUILD_FUZZ} "Failed to build ${TARGETS}"
popd

MachineFuzzTestCase="${SSPL_TOOLS_DIR}/${FuzzTestCaseRelDir}"
MachineExecPath="${CMAKE_BUILD_FULL_PATH}/eventjournaler/tests/FuzzerTests/AflFuzzScripts"
#LIBS_MACHINE="${CMAKE_BUILD_FULL_PATH}/libs"


VagrantFuzzTestCase=$(echo ${MachineFuzzTestCase} | sed s_${SSPL_TOOLS_DIR}_/vagrant_)
VagrantExecPath=$(echo ${MachineExecPath} | sed s_${SSPL_TOOLS_DIR}_/vagrant_)
#LIBS_VAGRANT=$(echo ${LIBS_MACHINE} | sed s_${SSPL_TOOLS_DIR}_/vagrant_)


pushd ${CMAKE_BUILD_FULL_PATH}/eventjournaler/tests/FuzzerTests/AflFuzzScripts

for target in ${TARGETS}; do

echo configuring ${target} script
cat > fuzzRun${target}.sh << EOF
CORE_PATTERN=\$(cat /proc/sys/kernel/core_pattern)
if [[ \${CORE_PATTERN} != "core" ]]; then
  echo \${CORE_PATTERN} > backup_core_pattern_option
  echo 'core' | sudo tee -a /proc/sys/kernel/core_pattern
fi
mkdir -p /tmp/base/etc/
AFL_SKIP_CPUFREQ=1   ${AFL_PATH}/afl-fuzz -i "${MachineFuzzTestCase}/${target}/" -o findings_${target} -m 400 "${MachineExecPath}/${target}"
EOF


cat > fuzzRun${target}InVagrant.sh << EOF
## ensure that the /proc/sys/kernel/core_pattern is set to core as it is required by the fuzzer
CORE_PATTERN=\$(cat /proc/sys/kernel/core_pattern)
if [[ \${CORE_PATTERN} != "core" ]]; then
  echo \${CORE_PATTERN} > backup_core_pattern_option
  echo 'core' | sudo tee -a /proc/sys/kernel/core_pattern
fi
mkdir -p /tmp/base/etc/
LD_LIBRARY_PATH=${LIBS_VAGRANT}  /vagrant/${AFL_BASE_NAME}/afl-fuzz -i "${VagrantFuzzTestCase}/${target}/" -o findings_${target} -m 400 "${VagrantExecPath}/${target}"

if [[ -f backup_core_pattern_option ]]; then
  cat backup_core_pattern_option | sudo tee -a /proc/sys/kernel/core_pattern
  rm backup_core_pattern_option
fi
EOF

chmod +x fuzzRun${target}.sh
done
