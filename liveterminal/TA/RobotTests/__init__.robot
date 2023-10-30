*** Settings ***
Documentation    Live Response plugin tests

Library   OperatingSystem
Library   /opt/test/inputs/common_test_libs/FullInstallerUtils.py

Suite Setup      Global Setup Tasks
Suite Teardown   Uninstall SSPL

Force Tags  LOAD2

*** Keywords ***
Global Setup Tasks
    Set Global Variable  ${TEST_INPUT_PATH}         /opt/test/inputs
    ${placeholder} =  Get Environment Variable      SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}          ${placeholder}
    ${placeholder} =  Get Environment Variable      SYSTEMPRODUCT_TEST_INPUT    default=${TEST_INPUT_PATH}
    Set Global Variable  ${SYSTEMPRODUCT_TEST_INPUT}  ${placeholder}
    Set Environment Variable  SOPHOS_INSTALL        ${SOPHOS_INSTALL}
    Set Global Variable  ${BASE_LOGS_DIR}           ${SOPHOS_INSTALL}/logs/base
    Set Global Variable  ${MCS_DIR}                 ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable  ${ETC_DIR}                 ${SOPHOS_INSTALL}/base/etc
    Set Global Variable  ${MANAGEMENT_AGENT}        ${SOPHOS_INSTALL}/base/bin/sophos_managementagent
    Set Global Variable  ${LIVERESPONSE_DIR}        ${SOPHOS_INSTALL}/plugins/liveresponse
    Set Global Variable  ${MCS_POLICY_CONFIG}       ${ETC_DIR}/sophosspl/mcs_policy.config
    Set Global Variable  ${COMMON_TEST_LIBS}        ${TEST_INPUT_PATH}/common_test_libs
    Set Global Variable  ${COMMON_TEST_ROBOT}       ${TEST_INPUT_PATH}/common_test_robot
    Set Global Variable  ${BASE_TEST_LIBS}          ${TEST_INPUT_PATH}/libs
    Set Global Variable  ${LIBS_DIRECTORY}          ${TEST_INPUT_PATH}/libs
    Set Global Variable  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}     ${TEST_INPUT_PATH}/libs
    Set Global Variable  ${SSPL_LIVERESPONSE_PLUGIN_SDDS}          ${TEST_INPUT_PATH}/liveresponse
    
    Set Global Variable  ${SUPPORT_FILES}           ${TEST_INPUT_PATH}/SupportFiles
    Set Global Variable  ${ROBOT_TESTS_DIR}         ${TEST_INPUT_PATH}/test_scripts/RobotTests
    Set Global Variable  ${WEBSOCKET_SERVER}        ${TEST_INPUT_PATH}/pytest_scripts/utils/websocket_server
    Set Global Variable  ${BASE_DIST}               ${TEST_INPUT_PATH}/base
