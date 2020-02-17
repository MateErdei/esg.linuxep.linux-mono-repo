*** Settings ***
Suite Setup     Global Setup Tasks
Suite Teardown  Global Teardown Tasks

Test Setup      Component Test Setup
Test Teardown   Component Test TearDown


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
    Set Global Variable  ${TEST_SCRIPTS_PATH}       ${TEST_INPUT_PATH}/test_scripts
    Set Global Variable  ${ROBOT_SCRIPTS_PATH}      ${TEST_SCRIPTS_PATH}/RobotScripts
    Set Global Variable  ${INTEGRATION_SCRIPTS_PATH}  ${TEST_SCRIPTS_PATH}/integration
    Set Global Variable  ${RESOURCES_PATH}          ${ROBOT_SCRIPTS_PATH}/resources
    Set Global Variable  ${COMPONENT_NAME}          av
    Set Global Variable  ${COMPONENT_SDDS}          ${TEST_INPUT_PATH}/${COMPONENT_NAME}/SDDS-COMPONENT
    Set Global Variable  ${COMPONENT_ROOT_PATH}     ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}
    Set Global Variable  ${COMPONENT_BIN_PATH}      ${COMPONENT_ROOT_PATH}/sbin/${COMPONENT_NAME}
    Set Global variable  ${COMPONENT_LIB64_DIR}     ${COMPONENT_ROOT_PATH}/lib64
    Set Global Variable  ${FAKEMANAGEMENT_AGENT_LOG_PATH}  ${SOPHOS_INSTALL}/tmp/fake_management_agent.log
    Set Global Variable  ${MANAGEMENT_AGENT_LOG_PATH}  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log

    Setup Base And Component


Global Teardown Tasks
    Remove Directory  ${SOPHOS_INSTALL}  recursive=True