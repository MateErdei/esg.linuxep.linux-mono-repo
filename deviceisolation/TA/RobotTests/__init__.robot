*** Settings ***
Suite Setup     Global Setup Tasks
Suite Teardown  Global Teardown Tasks

Library         Process
Library         OperatingSystem

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
    Set Global Variable  ${COMMON_TEST_ROBOT}       ${TEST_INPUT_PATH}/common_test_robot
    Set Global Variable  ${ROBOT_SCRIPTS_PATH}      ${TEST_INPUT_PATH}/test_scripts/RobotTests
    Set Global Variable  ${COMPONENT_NAME}          deviceisolation
    Set Global Variable  ${COMPONENT_SDDS}          ${TEST_INPUT_PATH}/device_isolation_sdds
    Set Global Variable  ${EXAMPLE_DATA_PATH}       ${ROBOT_SCRIPTS_PATH}/data
    Set Global Variable  ${COMPONENT_ROOT_PATH}     ${SOPHOS_INSTALL}/plugins/${COMPONENT_NAME}

    Directory Should Exist  ${ROBOT_SCRIPTS_PATH}
    Run Process    bash      ${COMMON_TEST_UTILS}/InstallCertificateToSystem.sh   ${COMMON_TEST_UTILS}/server_certs/server-root.crt

Global Teardown Tasks
    Remove Directory  ${SOPHOS_INSTALL}  recursive=True
    Run Process    bash    ${COMMON_TEST_UTILS}/CleanupInstalledSystemCerts.sh