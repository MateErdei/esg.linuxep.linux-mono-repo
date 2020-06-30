#!/usr/bin/env bash
set -ex

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

[[ -d ${BASE_DIST} ]]                                          || fail "Error: ${BASE_DIST} doesn't exist"
[[ -d ${EXAMPLEPLUGIN_SDDS} ]]                                 || fail "Error: ${EXAMPLEPLUGIN_SDDS} doesn't exist"
[[ -d ${SSPL_AUDIT_PLUGIN_SDDS} ]]                             || fail "Error: ${SSPL_AUDIT_PLUGIN_SDDS} doesn't exist"
[[ -d ${SSPL_PLUGIN_EVENTPROCESSOR_SDDS} ]]                    || fail "Error: ${SSPL_PLUGIN_EVENTPROCESSOR_SDDS} doesn't exist"
[[ -d ${SSPL_MDR_PLUGIN_SDDS} ]]                               || fail "Error: ${SSPL_MDR_PLUGIN_SDDS} doesn't exist"
[[ -d ${SSPL_EDR_PLUGIN_SDDS} ]]                               || fail "Error: ${SSPL_EDR_PLUGIN_SDDS} doesn't exist"
[[ -f ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} ]]                    || fail "Error: ${SYSTEM_PRODUCT_TEST_OUTPUT_FILE} doesn't exist"
[[ -d ${THIN_INSTALLER_OVERRIDE} ]]                            || fail "Error: ${THIN_INSTALLER_OVERRIDE} doesn't exist"
[[ -d ${SDDS_SSPL_DBOS_COMPONENT} ]]                           || fail "Error: ${SDDS_SSPL_DBOS_COMPONENT} doesn't exist"
[[ -d ${SDDS_SSPL_OSQUERY_COMPONENT} ]]                        || fail "Error: ${SDDS_SSPL_OSQUERY_COMPONENT} doesn't exist"
[[ -d ${SDDS_SSPL_MDR_COMPONENT} ]]                            || fail "Error: ${SDDS_SSPL_MDR_COMPONENT} doesn't exist"
[[ -d ${SDDS_SSPL_MDR_COMPONENT_SUITE} ]]                      || fail "Error: ${SDDS_SSPL_MDR_COMPONENT_SUITE} doesn't exist"
[[ -d ${SSPL_LIVERESPONSE_PLUGIN_SDDS} ]]                      || fail "Error: ${SSPL_LIVERESPONSE_PLUGIN_SDDS} doesn't exist"
[[ -d ${WEBSOCKET_SERVER} ]]                                   || fail "Error: ${WEBSOCKET_SERVER} doesn't exist"

[[ -d ${SSPL_BASE_SDDS_RELEASE_0_5} ]]                         || fail "Error: ${SSPL_BASE_SDDS_RELEASE_0_5} doesn't exist"
[[ -d ${SSPL_EXAMPLE_PLUGIN_SDDS_RELEASE_0_5} ]]               || fail "Error: ${SSPL_EXAMPLE_PLUGIN_SDDS_RELEASE_0_5} doesn't exist"

[[ -d ${OPENSSL_INPUT} ]]                                      || fail "Error: ${OPENSSL_INPUT} doesn't exist"
[[ -d ${CAPNP_INPUT} ]]                                        || fail "Error: ${CAPNP_INPUT} doesn't exist"

[[ -d ${SSPL_BASE_SDDS_RELEASE_1_0} ]]                         || fail "Error: ${SSPL_BASE_SDDS_RELEASE_1_0} doesn't exist"
[[ -d ${SDDS_SSPL_MDR_COMPONENT_SUITE_RELEASE_1_0} ]]          || fail "Error: ${SDDS_SSPL_MDR_COMPONENT_SUITE_RELEASE_1_0} doesn't exist"

[[ -d ${SAV_INPUT} ]]                                          || fail "Error: ${SAV_INPUT} doesn't exist"