*** Settings ***

Library         OperatingSystem
Library         Process
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

${TESTUSER}         testuser
${TESTGROUP}        testgroup

*** Keywords ***
Global Setup Tasks
    # SOPHOS_INSTALL
    ${placeholder} =  Get Environment Variable              SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}                  ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL                ${SOPHOS_INSTALL}

    Set Global Variable  ${TEST_SCRIPTS_PATH}               ${TEST_INPUT_PATH}/test_scripts
    Set Global Variable  ${LIBS_PATH}                       ${TEST_SCRIPTS_PATH}/Libs
    Set Global Variable  ${BASH_SCRIPTS_PATH}               ${LIBS_PATH}/bashScripts
    Set Global Variable  ${SUPPORT_FILES_PATH}              ${LIBS_PATH}/supportFiles
    Set Global Variable  ${RESOURCES_PATH}                  ${TEST_SCRIPTS_PATH}/resources
    Set Global Variable  ${BUILD_ARTEFACTS_FOR_TAP}         ${TEST_INPUT_PATH}/tap_test_output_from_build

    Set Global Variable  ${COMPONENT_SDDS_COMPONENT}        ${TEST_INPUT_PATH}/${COMPONENT_NAME}/SDDS-COMPONENT
    Set Global Variable  ${COMPONENT_SDDS}                  ${TEST_INPUT_PATH}/${COMPONENT_NAME}/INSTALL-SET
    Set Global Variable  ${COMPONENT_INSTALL_SET}           ${TEST_INPUT_PATH}/${COMPONENT_NAME}/INSTALL-SET
    Set Global Variable  ${COMPONENT_ROOT_PATH}             ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}
    Set Global Variable  ${COMPONENT_SBIN_DIR}              ${COMPONENT_ROOT_PATH}/sbin
    Set Global Variable  ${COMPONENT_VAR_DIR}               ${COMPONENT_ROOT_PATH}/var
    Set Global Variable  ${COMPONENT_SAFESTORE_DIR}         ${COMPONENT_ROOT_PATH}/safestore
    Set Global Variable  ${COMPONENT_BIN_PATH}              ${COMPONENT_SBIN_DIR}/${COMPONENT_NAME}
    Set Global variable  ${COMPONENT_LIB64_DIR}             ${COMPONENT_ROOT_PATH}/lib64
    Set Global Variable  ${FAKEMANAGEMENT_AGENT_LOG_PATH}   ${SOPHOS_INSTALL}/fake_management_agent.log
    Set Global Variable  ${MANAGEMENT_AGENT_LOG_PATH}       ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Set Global Variable  ${MCS_PATH}                        ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable  ${AV_PLUGIN_PATH}                  ${COMPONENT_ROOT_PATH}
    Set Global Variable  ${AV_LOG_PATH}                     ${AV_PLUGIN_PATH}/log/${COMPONENT}.log

    Set Global Variable  ${SAV_APPID}                       SAV
    Set Global Variable  ${ALC_APPID}                       ALC


    Set Global Variable  ${USING_FAKE_AV_SCANNER_FLAG}       UsingFakeAvScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  false
    Set Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL  1

    Create Install Set If Required
    CoreDumps.Enable Core Files

    Create test user and group
    Unpack TAP Tools

Global Teardown Tasks
    Run Keyword And Ignore Error  Uninstall All
    stop fake management if running

    Remove test user and group

Uninstall All
    Run Keyword And Ignore Error  Log File    /tmp/installer.log
    Run Keyword And Ignore Error  Log File   ${AV_LOG_PATH}
    LogUtils.dump_watchdog_log
    BaseUtils.uninstall_sspl_if_installed

Create test user and group
    Run Process  groupadd  ${TESTGROUP}
    Run Process  useradd  ${TESTUSER}  --no-create-home  --no-user-group  --gid  ${TESTGROUP}

Remove test user and group
    Run Process  /usr/sbin/userdel   ${TESTUSER}
    Run Process  /usr/sbin/groupdel   ${TESTGROUP}

Unpack TAP Tools
    ${result} =   Run Process    tar    xzf    ${BUILD_ARTEFACTS_FOR_TAP}/tap_test_output.tar.gz    -C    ${TEST_INPUT_PATH}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}
    Set Global Variable  ${AV_TEST_TOOLS}         ${TEST_INPUT_PATH}/tap_test_output