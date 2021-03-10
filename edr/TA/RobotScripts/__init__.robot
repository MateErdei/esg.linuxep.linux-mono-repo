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
    Set Global Variable  ${ROBOT_SCRIPTS_PATH}      ${TEST_INPUT_PATH}/test_scripts/RobotScripts
    Set Global Variable  ${EXAMPLE_DATA_PATH}       ${ROBOT_SCRIPTS_PATH}/data
    Set Global Variable  ${COMPONENT_NAME}          edr
    Set Global Variable  ${COMPONENT_SDDS}          ${TEST_INPUT_PATH}/edr_sdds
    Set Global Variable  ${COMPONENT_ROOT_PATH}     ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}
    Set Global Variable  ${COMPONENT_BIN_PATH}      ${COMPONENT_ROOT_PATH}/bin/${COMPONENT_NAME}
    Set Global variable  ${COMPONENT_LIB64_DIR}     ${COMPONENT_ROOT_PATH}/lib64
    Set Global Variable  ${FAKEMANAGEMENT_AGENT_LOG_PATH}  ${SOPHOS_INSTALL}/tmp/fake_management_agent.log

    Directory Should Exist  ${ROBOT_SCRIPTS_PATH}

Uninstall All
    Run Keyword And Ignore Error  Log File   /tmp/installer.log
    Run Keyword And Ignore Error  Log File   ${EDR_LOG_PATH}
    Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   shell=True   timeout=30s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall base.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"
    File Should Not Exist  /etc/rsyslog.d/rsyslog_sophos-spl.conf

Global Teardown Tasks
    Uninstall All
    Remove Directory  ${SOPHOS_INSTALL}  recursive=True