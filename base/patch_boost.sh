#!/bin/bash

# TODO LINUXDAR-1506: remove the patching when the related issue is incorporated into the released version of boost
# https://github.com/boostorg/process/issues/62

REDIST=$1
BASE=$2
echo "REDIST = $REDIST"
echo "BASE = $BASE"
BOOST_PROCESS_TARGET=$REDIST/boost/include/boost/process/detail/posix/executor.hpp
MODIFIED_VERSION=$BASE/patched_boost_executor.hpp

if [[ ! -f "$BOOST_PROCESS_TARGET" ]]
then
  echo "Could not patch boost, $BOOST_PROCESS_TARGET does not exist."
  exit 1
fi

if [[ ! -f "$MODIFIED_VERSION" ]]
then
  echo "Could not patch boost, $MODIFIED_VERSION does not exist."
  exit 1
fi

diff -u "$MODIFIED_VERSION" "$BOOST_PROCESS_TARGET" && DIFFERS=0 || DIFFERS=1
if [[ "${DIFFERS}" == "1" ]]
then
  cp "$MODIFIED_VERSION"  "$BOOST_PROCESS_TARGET"
  echo "Patched Boost executor"
else
  echo "Boost executor already patched"
fi
