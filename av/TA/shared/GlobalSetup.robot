*** Settings ***

Library         OperatingSystem
Library         ../Libs/InstallSet.py

*** Variables ***
${COMPONENT}        av
${COMPONENT_NAME}   ${COMPONENT}
${COMPONENT_UC}     AV
${TEST_INPUT_PATH}  /opt/test/inputs
${BASE_SDDS}        ${TEST_INPUT_PATH}/${COMPONENT}/base-sdds/

*** Keywords ***
Global Setup Tasks
    # SOPHOS_INSTALL
    ${placeholder} =  Get Environment Variable              SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}                  ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL                ${SOPHOS_INSTALL}
    Set Global Variable  ${TEST_INPUT_PATH}                 ${TEST_INPUT_PATH}
    Set Global Variable  ${TEST_SCRIPTS_PATH}               ${TEST_INPUT_PATH}/test_scripts
    Set Global Variable  ${INTEGRATION_TESTS_PATH}          ${TEST_SCRIPTS_PATH}/integration
    Set Global Variable  ${PRODUCT_TESTS_PATH}              ${TEST_SCRIPTS_PATH}/product
    Set Global Variable  ${RESOURCES_PATH}                  ${TEST_SCRIPTS_PATH}/resources
    Set Global Variable  ${COMPONENT_NAME}                  ${COMPONENT_NAME}
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

    Set Global Variable  ${USING_FAKE_AV_SCANNER_FLAG}            UsingFakeAvScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  false

    Create Install Set If Required



Global Teardown Tasks
    Run Keyword And Ignore Error  Remove Directory  ${SOPHOS_INSTALL}  recursive=True
