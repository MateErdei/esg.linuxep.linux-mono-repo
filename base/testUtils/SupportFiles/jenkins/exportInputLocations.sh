#!/usr/bin/env bash
set -e

[[ -z $SYSTEMPRODUCT_TEST_INPUT ]] && SYSTEMPRODUCT_TEST_INPUT=/tmp/system-product-test-inputs
if [[  -n $COVERAGE_BASE_BUILD ]]; then
  export BASE_DIST=$COVERAGE_BASE_BUILD
else
  export BASE_DIST="$SYSTEMPRODUCT_TEST_INPUT/sspl-base"
fi
export EXAMPLEPLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-exampleplugin"
export SSPL_EDR_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-edr-plugin"
export SSPL_EDR_PLUGIN_MANUAL_TOOLS="$SYSTEMPRODUCT_TEST_INPUT/sspl-edr-plugin-manual-tools"
export SSPL_EVENT_JOURNALER_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-plugin-event-journaler"
export SSPL_EVENT_JOURNALER_PLUGIN_MANUAL_TOOLS="$SYSTEMPRODUCT_TEST_INPUT/sspl-plugin-event-journaler-manual-tools"
export SSPL_ANTI_VIRUS_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-plugin-anti-virus"
export SSPL_MDR_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-mdr-control-plugin"
export SYSTEM_PRODUCT_TEST_OUTPUT="$SYSTEMPRODUCT_TEST_INPUT/SystemProductTestOutput"
export SYSTEM_PRODUCT_TEST_OUTPUT_FILE="$SYSTEM_PRODUCT_TEST_OUTPUT/SystemProductTestOutput.tar.gz"
export THIN_INSTALLER_OVERRIDE="$SYSTEMPRODUCT_TEST_INPUT/sspl-thininstaller"
export SSPL_LIVERESPONSE_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/liveterminal"
export WEBSOCKET_SERVER="$SYSTEMPRODUCT_TEST_INPUT/websocket_server"
export SSPL_RUNTIMEDETECTIONS_PLUGIN_SDDS="$SYSTEMPRODUCT_TEST_INPUT/sspl-runtimedetections-plugin"

export SSPL_BASE_SDDS_RELEASE_0_5="$SYSTEMPRODUCT_TEST_INPUT/sspl-base-0-5"
export SSPL_EXAMPLE_PLUGIN_SDDS_RELEASE_0_5="$SYSTEMPRODUCT_TEST_INPUT/sspl-exampleplugin-0-5"

export OPENSSL_INPUT=$SYSTEMPRODUCT_TEST_INPUT/openssllinux11/

export SSPL_BASE_SDDS_RELEASE_1_0=$SYSTEMPRODUCT_TEST_INPUT/sspl-base-1-0
export SDDS_SSPL_MDR_COMPONENT_SUITE_RELEASE_1_0=$SYSTEMPRODUCT_TEST_INPUT/sspl-mdr-componentsuite-1-0

export SAV_INPUT=$SYSTEMPRODUCT_TEST_INPUT/savlinux9-package
