*** Settings ***
Suite Setup     Global Setup Tasks
Suite Teardown  Global Teardown Tasks

Library         OperatingSystem

Test Timeout    5 minutes
*** Keywords ***
Global Setup Tasks

    ${placeholder} =  Get Environment Variable      SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}          ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL        ${SOPHOS_INSTALL}
    Set Global Variable  ${TEST_INPUT_PATH}         /opt/test/inputs
    Set Global Variable  ${ROBOT_SCRIPTS_PATH}      ${TEST_INPUT_PATH}/test_scripts/RobotTests
    Set Global Variable  ${COMPONENT_NAME}          eventjournaler
    Set Global Variable  ${COMPONENT_SDDS}          ${TEST_INPUT_PATH}/event_journaler_sdds
    Set Global Variable  ${EVENT_PUB_SUB_TOOL}      ${TEST_INPUT_PATH}/manual_tools/EventPubSub
    Set Global Variable  ${EVENT_READER_TOOL}       ${TEST_INPUT_PATH}/manual_tools/JournalReader
    Set Global Variable  ${EXAMPLE_DATA_PATH}       ${ROBOT_SCRIPTS_PATH}/data
    Set Global Variable  ${COMPONENT_ROOT_PATH}     ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}
    Set Global Variable  ${EVENT_JOURNAL_DIR}       ${COMPONENT_ROOT_PATH}/data/eventjournals

    Directory Should Exist  ${ROBOT_SCRIPTS_PATH}

Global Teardown Tasks
    Remove Directory  ${SOPHOS_INSTALL}  recursive=True