*** Settings ***
Suite Setup   Global Setup Tasks
Suite Teardown   Global Cleanup Tasks

Library    OperatingSystem
Library    Process
Library    CentralUtils.py
Library    FullInstallerUtils.py
Library    OSUtils.py
Library    PathManager.py
Library    TemporaryDirectoryManager.py
Library    UpdateServer.py
Library    WarehouseUtils.py

Resource    /opt/test/inputs/common_test_robot/GeneralTeardownResource.robot

Test Timeout    5 minutes
Test Teardown   General Test Teardown

*** Keywords ***
Global Setup Tasks
    Set Environment Variable    SOPHOS_DEBUG_UNINSTALL    1
    # SOPHOS_INSTALL
    ${placeholder} =  Get Environment Variable  SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}  ${placeholder}

    Uninstall_SSPL  ${SOPHOS_INSTALL}  ${True}

    ${CLOUD_IP} =  Get Central Ip
    Set Suite Variable    ${CLOUD_IP}     ${CLOUD_IP}   children=true

    ${placeholder} =  Get Environment Variable  SYSTEMPRODUCT_TEST_INPUT  default=/tmp/system-product-test-inputs
    Set Global Variable  ${SYSTEMPRODUCT_TEST_INPUT}  ${placeholder}

    ${placeholder} =  Get Environment Variable  THIN_INSTALLER_OVERRIDE  default=${SYSTEMPRODUCT_TEST_INPUT}/sspl-thininstaller
    Set Global Variable  ${THIN_INSTALLER_INPUT}  ${placeholder}

    ${placeholder} =  Get Environment Variable  SDDS3_Builder  default=${SYSTEMPRODUCT_TEST_INPUT}/sdds3/sdds3-builder
    Set Global Variable  ${SDDS3_Builder}  ${placeholder}

    ${placeholder} =  Get Environment Variable  WORKSPACE  default=/home/jenkins/workspace/
    Set Global Variable  ${JENKINS_WORKSPACE}  ${placeholder}

    Set Global Variable  ${SUL_DOWNLOADER}              ${SOPHOS_INSTALL}/base/bin/SulDownloader
    Set Global Variable  ${VERSIGPATH}                  ${SOPHOS_INSTALL}/base/update/versig
    Set Global Variable  ${MCS_ROUTER}                  ${SOPHOS_INSTALL}/base/bin/mcsrouter
    Set Global Variable  ${REGISTER_CENTRAL}            ${SOPHOS_INSTALL}/base/bin/registerCentral
    Set Global Variable  ${MANAGEMENT_AGENT}            ${SOPHOS_INSTALL}/base/bin/sophos_managementagent
    Set Global Variable  ${ETC_DIR}                     ${SOPHOS_INSTALL}/base/etc
    Set Global Variable  ${VAR_DIR}                     ${SOPHOS_INSTALL}/var
    Set Global Variable  ${MCS_DIR}                     ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable  ${MCS_TMP_DIR}                 ${MCS_DIR}/tmp
    Set Global Variable  ${BASE_LOGS_DIR}               ${SOPHOS_INSTALL}/logs/base
    Set Global Variable  ${MCS_ROUTER_LOG}              ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log
    Set Global Variable  ${MCS_ENVELOPE_LOG}            ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log
    Set Global Variable  ${SULDOWNLOADER_LOG_PATH}      ${BASE_LOGS_DIR}/suldownloader.log
    Set Global Variable  ${PLUGIN_REGISTRY_DIR}         ${SOPHOS_INSTALL}/base/pluginRegistry
    Set Global Variable  ${UPDATE_DIR}                  ${SOPHOS_INSTALL}/base/update
    Set Global Variable  ${EDR_DIR}                     ${SOPHOS_INSTALL}/plugins/edr
    Set Global Variable  ${RUNTIMEDETECTIONS_DIR}       ${SOPHOS_INSTALL}/plugins/runtimedetections
    Set Global Variable  ${AV_DIR}                      ${SOPHOS_INSTALL}/plugins/av
    Set Global Variable  ${LIVERESPONSE_DIR}            ${SOPHOS_INSTALL}/plugins/liveresponse
    Set Global Variable  ${EVENTJOURNALER_DIR}          ${SOPHOS_INSTALL}/plugins/eventjournaler
    Set Global Variable  ${RESPONSE_ACTIONS_DIR}        ${SOPHOS_INSTALL}/plugins/responseactions
    Set Global Variable  ${UPDATE_CONFIG}               ${UPDATE_DIR}/var/updatescheduler/update_config.json
    Set Global Variable  ${UPDATECACHE_CERT_PATH}       ${UPDATE_DIR}/updatecachecerts/cache_certificates.crt
    Set Global Variable  ${UPDATE_ROOTCERT_DIR}         ${UPDATE_DIR}/rootcerts
    Set Global Variable  ${SDDS3_OVERRIDE_FILE}         ${UPDATE_DIR}/var/sdds3_override_settings.ini
    Set Global Variable  ${UPGRADING_MARKER_FILE}       ${SOPHOS_INSTALL}/var/sophosspl/upgrade_marker_file
    Set Global Variable  ${SHS_STATUS_FILE}             ${MCS_DIR}/status/SHS_status.xml
    Set Global Variable  ${SHS_POLICY_FILE}             ${MCS_DIR}/internal_policy/internal_EPHEALTH.json
    Set Global Variable  ${CURRENT_PROXY_FILE}          ${ETC_DIR}/sophosspl/current_proxy
    Set Global Variable  ${MCS_CONFIG}                  ${ETC_DIR}/sophosspl/mcs.config
    Set Global Variable  ${MCS_POLICY_CONFIG}           ${ETC_DIR}/sophosspl/mcs_policy.config
    Set Global Variable  ${WD_ACTUAL_USER_GROUP_IDS}       ${ETC_DIR}/user-group-ids-actual.conf
    Set Global Variable  ${WD_REQUESTED_USER_GROUP_IDS}    ${ETC_DIR}/user-group-ids-requested.conf

    Set Global Variable    ${TEST_INPUT_PATH}    /opt/test/inputs
    Set Global Variable    ${SYS_TEST_LIBS}      /opt/sspl/libs
    Set Global Variable    ${SYS_TEST_COMMON_ROBOT}      /opt/sspl/robot
    Set Global Variable    ${LOCAL_TEST_LIBS}    /vagrant/esg.linuxep.linux-mono-repo/common/TA/libs
    Set Global Variable    ${LOCAL_TEST_ROBOT}    /vagrant/esg.linuxep.linux-mono-repo/common/TA/robot
    Set Global Variable    ${JENKINS_TEST_LIBS}    ${JENKINS_WORKSPACE}/base/testUtils/libs
    Set Global Variable    ${JENKINS_TEST_ROBOT}    ${JENKINS_WORKSPACE}/base/testUtils/robot

    ${isSystemTestRun}=    Directory Exists    ${SYS_TEST_LIBS}
    ${isLocalTestRun}=     Directory Exists    ${LOCAL_TEST_LIBS}
    ${isJenkinsTestRun}=   Directory Exists    ${JENKINS_TEST_LIBS}

    Set Global Variable  ${COMMON_TEST_LIBS}    ${TEST_INPUT_PATH}/common_test_libs
    Set Global Variable  ${COMMON_TEST_UTILS}    ${TEST_INPUT_PATH}/common_test_utils
    Set Global Variable  ${BASE_SDDS_SCRIPTS}  ${TEST_INPUT_PATH}/base_sdds_scripts
    Set Global Variable  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}    ${TEST_INPUT_PATH}/SystemProductTestOutput

    Set Global Variable  ${WATCHDOG_SERVICE}            sophos-spl
    Set Global Variable  ${UPDATE_SERVICE}              sophos-spl-update
    Set Global Variable  ${BASE_RIGID_NAME}             ServerProtectionLinux-Base
    Set Global Variable  ${EXAMPLE_PLUGIN_RIGID_NAME}   ServerProtectionLinux-Plugin-Example
    Set Global Variable  ${TEMPORARY_DIRECTORIES}       /tmp
    Set Global Variable  ${TEST_TEMP_DIR}       ${CURDIR}/temp

    Set Global Variable  ${SUPPORT_FILES}     ${TEST_INPUT_PATH}/SupportFiles
    Set Global Variable  ${ROBOT_TESTS_DIR}   ${TEST_INPUT_PATH}/test_scripts
    Set Global Variable  ${LIBS_DIRECTORY}    ${COMMON_TEST_LIBS}

    install_system_ca_cert    ${COMMON_TEST_UTILS}/server_certs/server-root.crt

    Set Global Variable    ${VUT_WAREHOUSE_ROOT}                 ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/repo
    Set Global Variable    ${VUT_LAUNCH_DARKLY}                  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly
    Set Global Variable    ${VUT_LAUNCH_DARKLY_999}              ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999
    Set Global Variable    ${DOGFOOD_WAREHOUSE_ROOT}             ${SYSTEMPRODUCT_TEST_INPUT}/sdds3-dogfood
    Set Global Variable    ${CURRENT_SHIPPING_WAREHOUSE_ROOT}    ${SYSTEMPRODUCT_TEST_INPUT}/sdds3-current_shipping

    Set Global Variable    ${SAFESTORE_TOOL_PATH}    ${SYSTEMPRODUCT_TEST_INPUT}/safestore_tools/ssr/ssr

Global Cleanup Tasks
    Cleanup Temporary Folders
    cleanup_system_ca_certs
