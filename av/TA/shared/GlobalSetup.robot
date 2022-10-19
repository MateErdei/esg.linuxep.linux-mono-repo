*** Settings ***

Library         OperatingSystem
Library         ../Libs/CoreDumps.py
Library         ../Libs/InstallSet.py
Library         ../Libs/LogUtils.py
Library         ../Libs/BaseUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/TeardownTools.py

*** Variables ***
${COMPONENT}        av
${COMPONENT_NAME}   ${COMPONENT}
${COMPONENT_UC}     AV

${TEST_INPUT_PATH}  /opt/test/inputs
${BASE_SDDS}        ${TEST_INPUT_PATH}/${COMPONENT}/base-sdds/

${CLEAN_RESULT}                    ${0}
${ERROR_RESULT}                    ${8}
${PASSWORD_PROTECTED_RESULT}       ${16}
${VIRUS_DETECTED_RESULT}           ${24}
${SCAN_ABORTED_WITH_THREAT}        ${25}
${SCAN_ABORTED}                    ${36}
${EXECUTION_INTERRUPTED}           ${40}

*** Keywords ***
Global Setup Tasks
    # SOPHOS_INSTALL
    ${placeholder} =  Get Environment Variable              SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}                  ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL                ${SOPHOS_INSTALL}

    Set Global Variable  ${TEST_SCRIPTS_PATH}               ${TEST_INPUT_PATH}/test_scripts
    Set Global Variable  ${INTEGRATION_TESTS_PATH}          ${TEST_SCRIPTS_PATH}/integration
    Set Global Variable  ${PRODUCT_TESTS_PATH}              ${TEST_SCRIPTS_PATH}/product
    Set Global Variable  ${RESOURCES_PATH}                  ${TEST_SCRIPTS_PATH}/resources
    Set Global Variable  ${BUILD_ARTEFACTS_FOR_TAP}         ${TEST_INPUT_PATH}/tap_test_output_from_build/

    Set Global Variable  ${COMPONENT_SDDS_COMPONENT}        ${TEST_INPUT_PATH}/${COMPONENT_NAME}/SDDS-COMPONENT
    Set Global Variable  ${COMPONENT_SDDS}                  ${TEST_INPUT_PATH}/${COMPONENT_NAME}/INSTALL-SET
    Set Global Variable  ${COMPONENT_INSTALL_SET}           ${TEST_INPUT_PATH}/${COMPONENT_NAME}/INSTALL-SET
    Set Global Variable  ${COMPONENT_ROOT_PATH}             ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}
    Set Global Variable  ${COMPONENT_SBIN_DIR}              ${COMPONENT_ROOT_PATH}/sbin
    Set Global Variable  ${COMPONENT_VAR_DIR}               ${COMPONENT_ROOT_PATH}/var
    Set Global Variable  ${COMPONENT_BIN_PATH}              ${COMPONENT_SBIN_DIR}/${COMPONENT_NAME}
    Set Global variable  ${COMPONENT_LIB64_DIR}             ${COMPONENT_ROOT_PATH}/lib64
    Set Global Variable  ${FAKEMANAGEMENT_AGENT_LOG_PATH}   ${SOPHOS_INSTALL}/fake_management_agent.log
    Set Global Variable  ${MANAGEMENT_AGENT_LOG_PATH}       ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Set Global Variable  ${MCS_PATH}                        ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable  ${AV_PLUGIN_PATH}                  ${COMPONENT_ROOT_PATH}
    Set Global Variable  ${AV_LOG_PATH}                     ${AV_PLUGIN_PATH}/log/${COMPONENT}.log


    Set Global Variable  ${USING_FAKE_AV_SCANNER_FLAG}       UsingFakeAvScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  false

    Create Install Set If Required
    CoreDumps.Enable Core Files

Global Teardown Tasks
    Run Keyword And Ignore Error  Uninstall All
    stop fake management if running


Uninstall All
    Run Keyword And Ignore Error  Log File    /tmp/installer.log
    Run Keyword And Ignore Error  Log File   ${AV_LOG_PATH}
    LogUtils.dump_watchdog_log
    BaseUtils.uninstall_sspl_if_installed
