*** Settings ***
Suite Setup   Global Setup Tasks
Suite Teardown   Global Cleanup Tasks

Library    ../libs/SystemProductTestOutputInstall.py
Library    ../libs/FullInstallerUtils.py
Library    ../libs/CentralUtils.py
Library    ../libs/TemporaryDirectoryManager.py
Library    ../libs/WarehouseUtils.py
Library    ../libs/OSUtils.py
Library    ../libs/UpdateServer.py
Library    ../libs/PathManager.py
Library           OperatingSystem

Resource   GeneralTeardownResource.robot

Test Timeout    5 minutes
Test Teardown   General Test Teardown

*** Keywords ***
Global Setup Tasks
    # SOPHOS_INSTALL
    ${placeholder} =  Get Environment Variable  SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}  ${placeholder}

    ${CLOUD_IP} =  Get Central Ip
    Set Suite Variable    ${CLOUD_IP}     ${CLOUD_IP}   children=true

    ${placeholder} =  Get Environment Variable  CAPNP_INPUT  default=/uk-filer5/prodro/bir/capnprotolinux11/0-7-0-29/213995/output
    Set Global Variable  ${CAPNP_INPUT}  ${placeholder}

    ${placeholder} =  Get Environment Variable  SAV_INPUT  default=/uk-filer5/prodro/bir/savlinux9-package/10-5-0/217084/savlinux-package
    Set Global Variable  ${SAV_INPUT}  ${placeholder}

    ${placeholder} =  Get Environment Variable  OPENSSL_INPUT  default=/uk-filer5/prodro/bir/openssllinux11/1-1-1c-16/216598/output
    Set Global Variable  ${OPENSSL_INPUT}  ${placeholder}

    Set Global Variable  ${SUL_DOWNLOADER}              ${SOPHOS_INSTALL}/base/bin/SulDownloader
    Set Global Variable  ${VERSIGPATH}                  ${SOPHOS_INSTALL}/base/update/versig
    Set Global Variable  ${MCS_ROUTER}                  ${SOPHOS_INSTALL}/base/bin/mcsrouter
    Set Global Variable  ${REGISTER_CENTRAL}            ${SOPHOS_INSTALL}/base/bin/registerCentral
    Set Global Variable  ${MANAGEMENT_AGENT}            ${SOPHOS_INSTALL}/base/bin/sophos_managementagent
    Set Global Variable  ${COMMS_COMPONENT}             ${SOPHOS_INSTALL}/base/bin/CommsComponent
    Set Global Variable  ${ETC_DIR}                     ${SOPHOS_INSTALL}/base/etc
    Set Global Variable  ${MCS_DIR}                     ${SOPHOS_INSTALL}/base/mcs
    Set Global Variable  ${BASE_LOGS_DIR}               ${SOPHOS_INSTALL}/logs/base
    Set Global Variable  ${PLUGIN_REGISTRY_DIR}         ${SOPHOS_INSTALL}/base/pluginRegistry
    Set Global Variable  ${UPDATE_DIR}                  ${SOPHOS_INSTALL}/base/update
    Set Global Variable  ${MTR_DIR}                     ${SOPHOS_INSTALL}/plugins/mtr
    Set Global Variable  ${EDR_DIR}                     ${SOPHOS_INSTALL}/plugins/edr
    Set Global Variable  ${LIVERESPONSE_DIR}            ${SOPHOS_INSTALL}/plugins/liveresponse
    Set Global Variable  ${UPDATE_CONFIG}               ${UPDATE_DIR}/var/updatescheduler/update_config.json
    Set Global Variable  ${UPDATECACHE_CERT_PATH}       ${UPDATE_DIR}/updatecachecerts/cache_certificates.crt
    Set Global Variable  ${UPDATE_ROOTCERT_DIR}         ${UPDATE_DIR}/rootcerts

    Set Global Variable  ${WATCHDOG_SERVICE}            sophos-spl
    Set Global Variable  ${UPDATE_SERVICE}              sophos-spl-update
    Set Global Variable  ${BASE_RIGID_NAME}             ServerProtectionLinux-Base
    Set Global Variable  ${EXAMPLE_PLUGIN_RIGID_NAME}   ServerProtectionLinux-Plugin-Example
    Set Global Variable  ${TEMPORARY_DIRECTORIES}       /tmp
    Set Global Variable  ${TEST_TEMP_DIR}       ${CURDIR}/temp

    ${placeholder} =  PathManager.get_support_file_path
    Set Global Variable  ${SUPPORT_FILES}     ${placeholder}
    ${placeholder} =  PathManager.get_robot_tests_path
    Set Global Variable  ${ROBOT_TESTS_DIR}     ${placeholder}
    ${placeholder}=  PathManager.get_libs_path
    Set Global Variable  ${LIBS_DIRECTORY}  ${placeholder}



    ${OPENSSL_FOLDER} =  Unpack Openssl   ${TEMPORARY_DIRECTORIES}
    Set Global Variable  ${OPENSSL_BIN_PATH}            ${TEMPORARY_DIRECTORIES}/openssl/bin64
    Set Global Variable  ${OPENSSL_LIB_PATH}            ${TEMPORARY_DIRECTORIES}/openssl/lib64

    # Check that the openssl unpacked otherwise suldownloader and updatescheduler tests may fail with
    # obscure openssl errors on Centos and Rhel
    Directory Should Exist  ${OPENSSL_BIN_PATH}
    Directory Should Exist  ${OPENSSL_LIB_PATH}

    ${system_product_test_tar_path}  ${system_product_test_output_path} =  Install System Product Test Output  ${CAPNP_INPUT}
    Set Global Variable  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}  ${system_product_test_output_path}
    ${colored_message} =  Evaluate  "\\033[33mUsing the following for system product test output ${system_product_test_tar_path}\\033[0m"
    Copy File  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/libcrypto.so.1.1        ${SUPPORT_FILES}/warehouseGeneration/lib
    Copy File  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/libssl.so.1.1           ${SUPPORT_FILES}/warehouseGeneration/lib
    Copy File  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/libssp.so.0.0.0         ${SUPPORT_FILES}/warehouseGeneration/lib/libssp.so.0
    Copy File  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/openssl                 ${SUPPORT_FILES}/warehouseGeneration/bin/openssl

    Log To Console  \n${colored_message} \n

    # Turn on the pub sub logging on all tests globally. The pub sub will then be logged in the management agent log
    # Should in general be false.
    Set Global Variable  ${PUB_SUB_LOGGING}  ${False}
    Run Keyword If   ${PUB_SUB_LOGGING}   Create Full Installer With Pub Sub Logging

    Generate Real Warehouse Alc Files
    Set Global Variable  ${GeneratedWarehousePolicies}  ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies

Global Cleanup Tasks
    Clean Up System Product Test Output
    Run Keyword If   ${PUB_SUB_LOGGING}   Clean Up Full Installer With Pub Sub Logging
    Cleanup Temporary Folders
