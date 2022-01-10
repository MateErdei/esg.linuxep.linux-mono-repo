*** Settings ***
Suite Setup     Global Setup Tasks
Suite Teardown  Global Teardown Tasks




Library         OperatingSystem
Resource        ComponentSetup.robot

Test Timeout    5 minutes


*** Keywords ***
Global Setup Tasks
    # SOPHOS_INSTALL

    ${placeholder} =  Get Environment Variable      SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}          ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL        ${SOPHOS_INSTALL}
    Set Global Variable  ${TEST_INPUT_PATH}         /opt/test/inputs
    Set Global Variable  ${ROBOT_SCRIPTS_PATH}      ${TEST_INPUT_PATH}/test_scripts/RobotScripts
    Set Global Variable  ${EXAMPLE_DATA_PATH}       ${ROBOT_SCRIPTS_PATH}/data
    Set Global Variable  ${INSTALL_SET_PATH}        ${ROBOT_SCRIPTS_PATH}/InstallSet
    Set Global Variable  ${COMPONENT_NAME}          edr
    Set Global Variable  ${COMPONENT_SDDS}          ${TEST_INPUT_PATH}/edr_sdds
    Set Global Variable  ${COMPONENT_ROOT_PATH}     ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}
    Set Global Variable  ${COMPONENT_BIN_PATH}      ${COMPONENT_ROOT_PATH}/bin/${COMPONENT_NAME}
    Set Global variable  ${COMPONENT_LIB64_DIR}     ${COMPONENT_ROOT_PATH}/lib64
    Set Global Variable  ${FAKEMANAGEMENT_AGENT_LOG_PATH}  /tmp/fake_management_agent.log

    Directory Should Exist  ${ROBOT_SCRIPTS_PATH}
    Create Directory  ${TEST_INPUT_PATH}/lp/
    Should Exist  ${TEST_INPUT_PATH}/lp_tar/sophos-query-pack.tar.gz
    ${result} =   Run Process   tar    xzf    ${TEST_INPUT_PATH}/lp_tar/sophos-query-pack.tar.gz   -C    ${TEST_INPUT_PATH}/lp/
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings   ${result.rc}  0

Global Teardown Tasks
    Uninstall All
    Remove Directory  ${SOPHOS_INSTALL}  recursive=True