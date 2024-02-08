*** Settings ***
Suite Setup     Global Setup Tasks
Suite Teardown  Global Teardown Tasks

Library         /opt/test/inputs/common_test_libs/OSUtils.py
Library         Process
Library         OperatingSystem

Library         ../Libs/InstallerUtils.py

Test Timeout    5 minutes

*** Keywords ***
Global Setup Tasks
    ${placeholder} =  Get Environment Variable      SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}          ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL        ${SOPHOS_INSTALL}
    Set Global Variable  ${BASE_LOGS_DIR}           ${SOPHOS_INSTALL}/logs/base
    Set Global Variable  ${MCS_DIR}                 ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable  ${TEST_INPUT_PATH}         /opt/test/inputs
    Set Global Variable  ${COMMON_TEST_LIBS}        ${TEST_INPUT_PATH}/common_test_libs
    Set Global Variable  ${COMMON_TEST_UTILS}       ${TEST_INPUT_PATH}/common_test_utils
    Set Global Variable  ${ROBOT_SCRIPTS_PATH}      ${TEST_INPUT_PATH}/test_scripts/RobotTests
    Set Global Variable  ${COMPONENT_NAME}          eventjournaler
    Set Global Variable  ${COMPONENT_SDDS}          ${TEST_INPUT_PATH}/event_journaler_sdds
    Set Global Variable  ${EVENT_PUB_SUB_TOOL}      ${TEST_INPUT_PATH}/manual_tools/EventPubSub
    Set Global Variable  ${EVENT_READER_TOOL}       ${TEST_INPUT_PATH}/manual_tools/JournalReader
    Set Global Variable  ${EXAMPLE_DATA_PATH}       ${ROBOT_SCRIPTS_PATH}/data
    Set Global Variable  ${COMPONENT_ROOT_PATH}     ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}
    Set Global Variable  ${EVENT_JOURNAL_DIR}       ${COMPONENT_ROOT_PATH}/data/eventjournals

    Directory Should Exist  ${ROBOT_SCRIPTS_PATH}

    Import Library   ${COMMON_TEST_LIBS}/OnFail.py
    Import Library   ${COMMON_TEST_LIBS}/CoreDumps.py

    CoreDumps.Enable Core Files  on fail dump logs

    install_system_ca_cert   ${COMMON_TEST_UTILS}/server_certs/server-root.crt

    OnFail.register_default_cleanup_action  CoreDumps.Check For Coredumps
    OnFail.register_default_cleanup_action  CoreDumps.Check Dmesg For Segfaults

    initial cleanup

Global Teardown Tasks
    Remove Directory  ${SOPHOS_INSTALL}  recursive=True
    cleanup_system_ca_certs