#!/usr/bin/env bash

# IMPORTANT: This script copies files from sspl-tools, base, event-journaler and liveterminal repos from your dev machine.
#            Please make sure that base, event-journaler and liveterminal are both up to date and on develop before running this script.

# This is a helper script that will copy the needed performance
# scripts from your machine to filer. This is to be used in conjunction
# with the sync_perf_machine.sh script which is to be executed, as root, on
# any machine which needs these performance scripts to run performance tests.

function failure()
{
  echo "$1"
  # The sleep is useful when running this from Clion on Windows because the bash window closes immediately on exit.
  sleep 5
  exit 1
}

function copy_file()
{
  cp "$THIS_DIR/$1" "$PERF_DIR/" || failure "Could not copy $THIS_DIR/$1 to $PERF_DIR/"
  echo "Copied $1"
}

function copy_dir()
{
  cp -r "$THIS_DIR/$1" "$PERF_DIR/" || failure "Could not copy $THIS_DIR/$1 to $PERF_DIR/"
  echo "Copied directory $1"
}

# Default to this is running on Linux.
PLATFORM="LINUX"
PERF_DIR=/mnt/filer6/linux/SSPL/performance
THIS_DIR=$(dirname "$0")
# Check if running on Windows under ming
if uname | grep -iq mingw
then
  PLATFORM="WIN"
fi

if [[ $PLATFORM == "WIN" ]]
then
  PERF_DIR=//uk-filer6.eng.sophos/Linux/SSPL/performance
fi

[[ -d $PERF_DIR ]] || failure "Could not access: $PERF_DIR"

copy_file PerformanceResources.py
copy_file RTD.sh
copy_file RunCentralLiveQuery.py
copy_file RunEventJournalerIngestionTest.py
copy_file RunLocalLiveQuery.py
copy_file RunPerfTests.py
copy_file build-gcc-only.sh
copy_file install-edr-mtr.sh
copy_file install-edr.sh
copy_file record_sspl_install_size.sh
copy_file run-test-gcc.sh
copy_file save-osquery-db-file-count.sh
copy_file sync_perf_machine.sh
copy_file ssplDogfoodFeedback.sh

copy_file ../everest-base/testUtils/SupportFiles/CloudAutomation/cloudClient.py
copy_file ../everest-base/testUtils/SupportFiles/CloudAutomation/SophosHTTPSClient.py

copy_file ../sspl-plugin-event-journaler/output/manualTools/JournalReader
copy_file ../sspl-plugin-event-journaler/output/manualTools/EventPubSub

# Live Response terminal tests
copy_file RunLocalLiveTerminal.py
copy_file 1000Chars
copy_dir ../liveterminal/ta/scripts/utils/websocket_server

echo -e "\nDone copying to: $PERF_DIR"