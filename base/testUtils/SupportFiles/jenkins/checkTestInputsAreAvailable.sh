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
check_exists ${EXAMPLEPLUGIN_SDDS}
check_exists ${SSPL_AUDIT_PLUGIN_SDDS}
check_exists ${SSPL_PLUGIN_EVENTPROCESSOR_SDDS}
check_exists ${SSPL_MDR_PLUGIN_SDDS}
check_exists ${SSPL_EDR_PLUGIN_SDDS}
[[ -f ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} ]]  || fail "Error: ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} doesn't exist"
check_exists ${THIN_INSTALLER_OVERRIDE}
check_exists ${SDDS_SSPL_DBOS_COMPONENT}
check_exists ${SDDS_SSPL_OSQUERY_COMPONENT}
check_exists ${SDDS_SSPL_MDR_COMPONENT}
check_exists ${SDDS_SSPL_MDR_COMPONENT_SUITE}
check_exists ${SSPL_LIVERESPONSE_PLUGIN_SDDS}
check_exists ${WEBSOCKET_SERVER}

check_exists ${SSPL_BASE_SDDS_RELEASE_0_5}
check_exists ${SSPL_EXAMPLE_PLUGIN_SDDS_RELEASE_0_5}

check_exists ${OPENSSL_INPUT}
check_exists ${CAPNP_INPUT}

check_exists ${SSPL_BASE_SDDS_RELEASE_1_0}
check_exists ${SDDS_SSPL_MDR_COMPONENT_SUITE_RELEASE_1_0}

check_exists ${SAV_INPUT}