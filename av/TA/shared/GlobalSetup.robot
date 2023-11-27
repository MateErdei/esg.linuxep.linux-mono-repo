*** Settings ***

Library         OperatingSystem
Library         Process
Library         /opt/test/inputs/common_test_libs/OSUtils.py
Library         ../Libs/PathManager.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/InstallSet.py
Library         ../Libs/LogUtils.py
Library         ../Libs/BaseUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/TeardownTools.py
Library    ${COMMON_TEST_LIBS}/DownloadAVSupplements.py

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
    Add Ta Dir To Sys Path
    # SOPHOS_INSTALL
    ${placeholder} =  Get Environment Variable              SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}                  ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL                ${SOPHOS_INSTALL}

    Set Global Variable  ${TEST_SCRIPTS_PATH}               ${TEST_INPUT_PATH}/test_scripts
    Set Global Variable  ${COMMON_TEST_LIBS}                ${TEST_INPUT_PATH}/common_test_libs
    Set Global Variable  ${COMMON_TEST_UTILS}               ${TEST_INPUT_PATH}/common_test_utils
    Set Global Variable  ${LIBS_PATH}                       ${TEST_SCRIPTS_PATH}/Libs
    Set Global Variable  ${BASH_SCRIPTS_PATH}               ${LIBS_PATH}/bashScripts
    Set Global Variable  ${SUPPORT_FILES_PATH}              ${LIBS_PATH}/supportFiles
    Set Global Variable  ${RESOURCES_PATH}                  ${TEST_SCRIPTS_PATH}/resources

    Set Global Variable  ${COMPONENT_SDDS_COMPONENT}        ${TEST_INPUT_PATH}/${COMPONENT_NAME}/SDDS-COMPONENT
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
    Set Global Variable  ${OA_LOCAL_SETTINGS}               ${COMPONENT_VAR_DIR}/on_access_local_settings.json

    Set Global Variable  ${SAV_APPID}                       SAV
    Set Global Variable  ${ALC_APPID}                       ALC


    Set Global Variable  ${USING_FAKE_AV_SCANNER_FLAG}       UsingFakeAvScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  false
    Set Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL  1

    Set Environment Variable    SDDS3_BUILDER    ${TEST_INPUT_PATH}/sdds3_tools/sdds3-builder
    Set Global Variable    ${SDDS3_BUILDER}    ${TEST_INPUT_PATH}/sdds3_tools/sdds3-builder
    Run Process    chmod    +x    ${SDDS3_BUILDER}
    Download Av Supplements

    Create Install Set If Required
    CoreDumps.Enable Core Files

    Create test user and group
    install_system_ca_cert   ${COMMON_TEST_UTILS}/server_certs/server-root.crt

Global Teardown Tasks
    Run Keyword And Ignore Error  Uninstall All
    stop fake management if running

    Remove test user and group
    cleanup_system_ca_certs

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
