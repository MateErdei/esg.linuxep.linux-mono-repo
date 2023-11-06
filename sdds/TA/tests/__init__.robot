*** Settings ***
Documentation    Setup for tests

Suite Setup   Global Setup Tasks

Library           OperatingSystem
Library           Process

Library    ../libs/WarehouseUtils.py

*** Keywords ***
Global Setup Tasks
    ${placeholder} =  Get Environment Variable    INPUT_DIRECTORY    default=/opt/test/inputs
    Set Global Variable  ${INPUT_DIRECTORY}     ${placeholder}
    ${placeholder} =  Get Environment Variable    TEST_SCRIPT_PATH    default=${INPUT_DIRECTORY}/test_scripts
    Set Global Variable  ${TEST_SCRIPT_PATH}     ${placeholder}
    ${placeholder} =  Get Environment Variable    SUPPORT_FILES    default=${INPUT_DIRECTORY}/SupportFiles
    Set Global Variable  ${SUPPORT_FILES}     ${placeholder}
    ${placeholder} =  Get Environment Variable    COMMON_TEST_LIBS    default=${INPUT_DIRECTORY}/common_test_libs
    Set Global Variable  ${COMMON_TEST_LIBS}     ${placeholder}
    Set Global Variable  ${LIBS_DIRECTORY}     ${placeholder}
    Set Global Variable  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}     ${TEST_SCRIPT_PATH}/libs
    ${placeholder} =  Get Environment Variable    COMMON_TEST_ROBOT    default=${INPUT_DIRECTORY}/common_test_robot
    Set Global Variable  ${COMMON_TEST_ROBOT}     ${placeholder}
    ${placeholder} =  Get Environment Variable    SOPHOS_INSTALL    default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}  ${placeholder}
    ${placeholder} =  Get Environment Variable    SYSTEMPRODUCT_TEST_INPUT    default=${INPUT_DIRECTORY}
    Set Global Variable  ${SYSTEMPRODUCT_TEST_INPUT}  ${placeholder}

    Set Global Variable    ${PLUGINS_DIR}                 ${SOPHOS_INSTALL}/plugins
    Set Global Variable    ${SUL_DOWNLOADER}              ${SOPHOS_INSTALL}/base/bin/SulDownloader
    Set Global Variable    ${VERSIGPATH}                  ${SOPHOS_INSTALL}/base/update/versig
    Set Global Variable    ${MCS_ROUTER}                  ${SOPHOS_INSTALL}/base/bin/mcsrouter
    Set Global Variable    ${REGISTER_CENTRAL}            ${SOPHOS_INSTALL}/base/bin/registerCentral
    Set Global Variable    ${MANAGEMENT_AGENT}            ${SOPHOS_INSTALL}/base/bin/sophos_managementagent
    Set Global Variable    ${ETC_DIR}                     ${SOPHOS_INSTALL}/base/etc
    Set Global Variable    ${VAR_DIR}                     ${SOPHOS_INSTALL}/var
    Set Global Variable    ${MCS_DIR}                     ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable    ${MCS_TMP_DIR}                 ${MCS_DIR}/tmp
    Set Global Variable    ${BASE_LOGS_DIR}               ${SOPHOS_INSTALL}/logs/base
    Set Global Variable    ${MCS_ROUTER_LOG}              ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log
    Set Global Variable    ${MCS_ENVELOPE_LOG}            ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log
    Set Global Variable    ${SULDOWNLOADER_LOG_PATH}      ${BASE_LOGS_DIR}/suldownloader.log
    Set Global Variable    ${PLUGIN_REGISTRY_DIR}         ${SOPHOS_INSTALL}/base/pluginRegistry
    Set Global Variable    ${UPDATE_DIR}                  ${SOPHOS_INSTALL}/base/update
    Set Global Variable    ${UPDATE_CONFIG}               ${UPDATE_DIR}/var/updatescheduler/update_config.json
    Set Global Variable    ${UPDATECACHE_CERT_PATH}       ${UPDATE_DIR}/updatecachecerts/cache_certificates.crt
    Set Global Variable    ${UPDATE_ROOTCERT_DIR}         ${UPDATE_DIR}/rootcerts
    Set Global Variable    ${SDDS3_OVERRIDE_FILE}         ${UPDATE_DIR}/var/sdds3_override_settings.ini
    Set Global Variable    ${UPGRADING_MARKER_FILE}       ${SOPHOS_INSTALL}/var/sophosspl/upgrade_marker_file
    Set Global Variable    ${SHS_STATUS_FILE}             ${MCS_DIR}/status/SHS_status.xml
    Set Global Variable    ${SHS_POLICY_FILE}             ${MCS_DIR}/internal_policy/internal_EPHEALTH.json
    Set Global Variable    ${CURRENT_PROXY_FILE}          ${ETC_DIR}/sophosspl/current_proxy
    Set Global Variable    ${MCS_CONFIG}                  ${ETC_DIR}/sophosspl/mcs.config
    Set Global Variable    ${MCS_POLICY_CONFIG}           ${ETC_DIR}/sophosspl/mcs_policy.config
    Set Global Variable    ${WD_ACTUAL_USER_GROUP_IDS}       ${ETC_DIR}/user-group-ids-actual.conf
    Set Global Variable    ${WD_REQUESTED_USER_GROUP_IDS}    ${ETC_DIR}/user-group-ids-requested.conf

    Set Global Variable    ${AV_DIR}                      ${PLUGINS_DIR}/av
    Set Global Variable    ${EDR_DIR}                     ${PLUGINS_DIR}/edr
    Set Global Variable    ${EVENTJOURNALER_DIR}          ${PLUGINS_DIR}/eventjournaler
    Set Global Variable    ${LIVERESPONSE_DIR}            ${PLUGINS_DIR}/liveresponse
    Set Global Variable    ${RESPONSE_ACTIONS_DIR}        ${PLUGINS_DIR}/responseactions
    Set Global Variable    ${RTD_DIR}                     ${PLUGINS_DIR}/runtimedetections
    Set Global Variable    ${RUNTIMEDETECTIONS_DIR}       ${PLUGINS_DIR}/runtimedetections

    Set Global Variable    ${SAFESTORE_DB_DIR}                 ${AV_DIR}/var/safestore_db
    Set Global Variable    ${SAFESTORE_DB_PATH}                ${SAFESTORE_DB_DIR}/safestore.db
    Set Global Variable    ${SAFESTORE_DB_PASSWORD_PATH}       ${SAFESTORE_DB_DIR}/safestore.pw

    ${placeholder} =  Get Environment Variable    INPUT_DIRECTORY    default=/opt/test/inputs
    Set Global Variable    ${INPUT_DIRECTORY}             ${placeholder}
    Set Global Variable    ${THIN_INSTALLER_DIRECTORY}    ${INPUT_DIRECTORY}/thin_installer
    Set Global Variable    ${SDDS3_Builder}               ${INPUT_DIRECTORY}/sdds3/sdds3-builder
    Set Global Variable    ${BASE_SDDS_SCRIPTS}           ${INPUT_DIRECTORY}/base_sdds_scripts

    Run Process    chmod    +x
    ...    ${SUPPORT_FILES}/openssl/openssl
    ...    ${SUPPORT_FILES}/openssl/libcrypto.so.3
    ...    ${SUPPORT_FILES}/openssl/libssl.so.3
    ...    ${SDDS3_Builder}
    Set Global Variable    ${OPENSSL_BIN_PATH}    ${SUPPORT_FILES}/openssl
    Set Global Variable    ${OPENSSL_LIB_PATH}    ${SUPPORT_FILES}/openssl
    Set Global Variable    ${TEST_TEMP_DIR}       ${CURDIR}/temp

    ${placeholder} =  Get Environment Variable    VUT_WAREHOUSE_ROOT    default=${INPUT_DIRECTORY}/repo
    Set Global Variable    ${VUT_WAREHOUSE_ROOT}                 ${placeholder}
    ${placeholder} =  Get Environment Variable    DOGFOOD_WAREHOUSE_REPO_ROOT    default=${INPUT_DIRECTORY}/dogfood_repo
    Set Global Variable    ${DOGFOOD_WAREHOUSE_REPO_ROOT}             ${placeholder}
    ${placeholder} =  Get Environment Variable    CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT    default=${INPUT_DIRECTORY}/current_shipping_repo
    Set Global Variable    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}    ${placeholder}
    ${placeholder} =  Get Environment Variable    VUT_LAUNCH_DARKLY    default=${INPUT_DIRECTORY}/launchdarkly
    Set Global Variable    ${VUT_LAUNCH_DARKLY}                 ${placeholder}
    ${placeholder} =  Get Environment Variable    STATIC_SUITES    default=${INPUT_DIRECTORY}/static_suites
    Set Global Variable    ${STATIC_SUITES}                     ${placeholder}
    ${placeholder} =  Get Environment Variable    DOGFOOD_LAUNCH_DARKLY    default=${INPUT_DIRECTORY}/dogfood_launch_darkly
    Set Global Variable    ${DOGFOOD_LAUNCH_DARKLY}             ${placeholder}
    ${placeholder} =  Get Environment Variable    CURRENT_LAUNCH_DARKLY    default=${INPUT_DIRECTORY}/current_shipping_launch_darkly
    Set Global Variable    ${CURRENT_LAUNCH_DARKLY}             ${placeholder}
    ${placeholder} =  Get Environment Variable    VUT_LAUNCH_DARKLY_999    default=${INPUT_DIRECTORY}/launchdarkly999
    Set Global Variable    ${VUT_LAUNCH_DARKLY_999}             ${placeholder}
    ${placeholder} =  Get Environment Variable    STATIC_SUITES_999        default=${INPUT_DIRECTORY}/static_suites999
    Set Global Variable    ${STATIC_SUITES_999}                 ${placeholder}
    ${placeholder} =  Get Environment Variable  SSPL_EVENT_JOURNALER_PLUGIN_MANUAL_TOOLS  default=${INPUT_DIRECTORY}/ej_manual_tools/JournalReader
    Set Global Variable  ${EVENT_JOURNALER_TOOLS}  ${placeholder}

    Set Global Variable    ${SAFESTORE_TOOL_PATH}    ${INPUT_DIRECTORY}/safestore_tools/ssr/ssr
