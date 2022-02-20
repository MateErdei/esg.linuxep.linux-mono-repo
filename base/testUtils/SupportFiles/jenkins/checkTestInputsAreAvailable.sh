#!/usr/bin/env bash
set -ex

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

function check_exists
{
    [[ -d ${1} ]]  || fail "Error: ${1} doesn't exist"
}

check_exists ${BASE_DIST}

check_exists ${SSPL_MDR_PLUGIN_SDDS}
check_exists ${SSPL_EDR_PLUGIN_SDDS}
check_exists ${SSPL_EVENT_JOURNALER_PLUGIN_SDDS}
check_exists ${SSPL_EVENT_JOURNALER_PLUGIN_MANUAL_TOOLS}

[[ -f ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} ]]  || fail "Error: ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} doesn't exist"
check_exists ${THIN_INSTALLER_OVERRIDE}
check_exists ${SDDS_SSPL_MDR_COMPONENT}
check_exists ${SDDS_SSPL_MDR_COMPONENT_SUITE}
check_exists ${SSPL_LIVERESPONSE_PLUGIN_SDDS}
check_exists ${WEBSOCKET_SERVER}

check_exists ${OPENSSL_INPUT}

check_exists ${SAV_INPUT}