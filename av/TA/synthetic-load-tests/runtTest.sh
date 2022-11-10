#!/bin/bash

set -ex

## Overview:
# 2. Disable update-scheduler
# 3. Disable on-access
# 4. Run phoronix test suite
# 5. Enable on-access
# 6. Run phoronix test suite
# 7. Disable entire sspl suite
# 8. Run phoronix test suite
# 9. Reset to original state
# 10. Upload data to Graphana
# 5. Enable update-scheduler

function failure()
{
    echo "$@"
    exit 1
}

function disable_update_scheduler()
{
    "$SOPHOS_INSTALL"/bin/wdctl stop updatescheduler || failure "Failed to stop update scheduler"
}

function enable_update_scheduler()
{
    # shellcheck disable=SC2086
    $SOPHOS_INSTALL/bin/wdctl start updatescheduler || failure "Failed to stop update scheduler"
}

function disable_msc_router()
{
    "$SOPHOS_INSTALL"/bin/wdctl stop mcsrouter || failure "Failed to stop mcs"
}

function enable_msc_router()
{
    "$SOPHOS_INSTALL"/bin/wdctl start mcsrouter  || failure "Failed to start mcs"
}

function restart_on_access()
{
    "$SOPHOS_INSTALL"/bin/wdctl stop  on_access_process || failure "Failed to stop on-access"
    "$SOPHOS_INSTALL"/bin/wdctl start  on_access_process || failure "Failed to start on-access"
}

function disable_on_access()
{
    cp "$BASE"/resources/oa_config_enabled.json /opt/sophos-spl/plugins/av/var/oa_flag.json
    restart_on_access
}

function enable_on_access()
{
    cp "$BASE"/resources/oa_config_enabled.json /opt/sophos-spl/plugins/av/var/oa_flag.json
    restart_on_access
}

function test_setup()
{
    disable_update_scheduler
    disable_msc_router
}

function test_teardown()
{
    enable_msc_router
    enable_update_scheduler
}

function disable_sspl()
{
    systemctl stop sophos-spl
}

function enable_sspl()
{
    systemctl start sophos-spl
}

SOPHOS_INSTALL=${SOPHOS_INSTALL:-/opt/sophos-spl}
cd ${0%/*}
BASE=$(pwd)


function start_on_access_enabled_tests()
{
  test_setup
  enable_on_access

  TEST_RESULTS_NAME="oa-enabled-synthetic-test-$(date +%m-%d-%Y)" TEST_RESULTS_IDENTIFIER="$(date)" \
  TEST_RESULTS_DESCRIPTION="On access testing using the Phoronix Test Suite." \
  FORCE_TIMES_TO_RUN=2 \
  phoronix-test-suite batch-benchmark \
  pts/node-express-loadtest pts/postmark pts/redis-1.3.1 pts/apache-1.7.2

  disable_on_access
  test_teardown
}

function start_sophos_disabled_tests()
{
  disable_sspl

  TEST_RESULTS_NAME="sophos-disabled-synthetic-test-$(date +%m-%d-%Y)" \
  TEST_RESULTS_IDENTIFIER="$(date)" \
  TEST_RESULTS_DESCRIPTION="On access testing using the Phoronix Test Suite." \
  FORCE_TIMES_TO_RUN=2 \
  phoronix-test-suite batch-benchmark \
  pts/node-express-loadtest pts/postmark pts/redis-1.3.1 pts/apache-1.7.2

  enable_sspl
}

function start_on_access_disabled_tests()
{
  test_setup
  disable_on_access

  TEST_RESULTS_NAME="oa-disabled-synthetic-test-$(date +%m-%d-%Y)" TEST_RESULTS_IDENTIFIER="$(date)" \
  TEST_RESULTS_DESCRIPTION="On access testing using the Phoronix Test Suite." \
  FORCE_TIMES_TO_RUN=2 \
  phoronix-test-suite batch-benchmark \
  pts/node-express-loadtest pts/postmark pts/redis-1.3.1 pts/apache-1.7.2

  test_teardown
}

start_on_access_disabled_tests