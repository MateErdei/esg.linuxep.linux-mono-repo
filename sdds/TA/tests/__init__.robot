*** Settings ***
Documentation    Setup for tests

Suite Setup   Global Setup Tasks

Library           OperatingSystem
Library           Process

Library    ../libs/WarehouseUtils.py

*** Keywords ***
Global Setup Tasks
    ${placeholder} =  Get Environment Variable    TEST_SCRIPT_PATH    default=/opt/test/inputs/test_scripts
    Set Global Variable  ${TEST_SCRIPT_PATH}     ${placeholder}
    ${placeholder} =  Get Environment Variable    SUPPORT_FILES    default=${TEST_SCRIPT_PATH}/SupportFiles
    Set Global Variable  ${SUPPORT_FILES}     ${placeholder}
    ${placeholder} =  Get Environment Variable    LIB_FILES    default=${TEST_SCRIPT_PATH}/libs
    Set Global Variable  ${LIB_FILES}     ${placeholder}
    ${placeholder} =  Get Environment Variable    SOPHOS_INSTALL    default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}  ${placeholder}

    Set Global Variable    ${BASE_LOGS_DIR}               ${SOPHOS_INSTALL}/logs/base
    Set Global Variable    ${MCS_DIR}                     ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable    ${PLUGINS_DIR}                 ${SOPHOS_INSTALL}/plugins

    Set Global Variable    ${AV_DIR}                      ${PLUGINS_DIR}/av
    Set Global Variable    ${EDR_DIR}                     ${PLUGINS_DIR}/edr
    Set Global Variable    ${EVENTJOURNALER_DIR}          ${PLUGINS_DIR}/eventjournaler
    Set Global Variable    ${LIVERESPONSE_DIR}            ${PLUGINS_DIR}/liveresponse
    Set Global Variable    ${RESPONSE_ACTIONS_DIR}        ${PLUGINS_DIR}/responseactions
    Set Global Variable    ${RTD_DIR}                     ${PLUGINS_DIR}/runtimedetections
    Set Global Variable    ${SULDOWNLOADER_LOG_PATH}      ${BASE_LOGS_DIR}/suldownloader.log

    Set Global Variable    ${SAFESTORE_DB_DIR}                 ${AV_DIR}/var/safestore_db
    Set Global Variable    ${SAFESTORE_DB_PATH}                ${SAFESTORE_DB_DIR}/safestore.db
    Set Global Variable    ${SAFESTORE_DB_PASSWORD_PATH}       ${SAFESTORE_DB_DIR}/safestore.pw

    ${placeholder} =  Get Environment Variable    INPUT_DIRECTORY    default=/opt/test/inputs
    Set Global Variable    ${INPUT_DIRECTORY}             ${placeholder}
    Set Global Variable    ${THIN_INSTALLER_DIRECTORY}    ${INPUT_DIRECTORY}/thin_installer

    ${placeholder} =  Get Environment Variable    BASE_TEST_UTILS    default=${INPUT_DIRECTORY}/base_test_utils
    Set Global Variable    ${BASE_TEST_UTILS}     ${placeholder}
    Set Global Variable    ${OPENSSL_BIN_PATH}    ${BASE_TEST_UTILS}/SystemProductTestOutput
    Set Global Variable    ${OPENSSL_LIB_PATH}    ${BASE_TEST_UTILS}/SystemProductTestOutput


    ${placeholder} =  Get Environment Variable    VUT_WAREHOUSE_REPO_ROOT    default=${INPUT_DIRECTORY}/repo
    Set Global Variable    ${VUT_WAREHOUSE_REPO_ROOT}                 ${placeholder}
    ${placeholder} =  Get Environment Variable    DOGFOOD_WAREHOUSE_REPO_ROOT    default=${INPUT_DIRECTORY}/dogfood_repo
    Set Global Variable    ${DOGFOOD_WAREHOUSE_REPO_ROOT}             ${placeholder}
    ${placeholder} =  Get Environment Variable    CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT    default=${INPUT_DIRECTORY}/current_shipping_repo
    Set Global Variable    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}    ${placeholder}

    Set Global Variable    ${SAFESTORE_TOOL_PATH}    ${INPUT_DIRECTORY}/safestore_tools/ssr/ssr
