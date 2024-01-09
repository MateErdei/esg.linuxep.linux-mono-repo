*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Thininstaller Test Teardown

Suite Setup      Setup sdds3 Update Tests
Suite Teardown   Cleanup sdds3 Update Tests

Library     ${COMMON_TEST_LIBS}/UpdateServer.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py
Library     ${COMMON_TEST_LIBS}/MCSRouter.py
Library     ${COMMON_TEST_LIBS}/CentralUtils.py
Library     Process
Library     DateTime
Library     OperatingSystem

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot
Resource    ${COMMON_TEST_ROBOT}/ThinInstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Force Tags  THIN_INSTALLER  thin_installer_main

*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir
@{FORCE_ARGUMENT} =  --force
@{PRODUCT_MDR_ARGUMENT} =  --products\=mdr
${BaseVUTPolicy}                    ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml

*** Test Case ***
Thin Installer Installs Working Base But Failing Plugin And Base Is Persisted While Plugin Error Is Logged
    Clean up fake warehouse
    Generate Fake Sdds3 Warehouse With Real Base But Fake Failing Plugin

    Should Not Exist    ${SOPHOS_INSTALL}
    Run Default Thininstaller  expected_return_code=${18}

    Check Thininstaller Log Contains    ERROR Installation of ServerProtectionLinux-Plugin-AV failed.
    Check Thininstaller Log Contains    ERROR ServerProtectionLinux-Plugin-AV: THIS IS BAD
    Check Thininstaller Log Contains    ERROR See ServerProtectionLinux-Plugin-AV_install.log for the full installer output.
    Check Thininstaller Log Contains    Failed to install at least one of the packages, see logs for details
    Check Thininstaller Log Contains    Warning: the product has been partially installed and not all functionality may be present
    Check Thininstaller Log Contains    Logs are available at /opt/sophos-spl/logs and /opt/sophos-spl/plugins/*/log
    Check Thininstaller Log Contains    Please see https://support.sophos.com/support/s/article/KB-000041952 for troubleshooting help

    Check Installed Correctly
    File should exist       ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-AV_install_failed.log

Thin Installer Installs Failing Base And Fails And Base Error is Logged And Product Is Removed
    Clean up fake warehouse
    Generate Fake Sdds3 Warehouse With Fake Failing Base

    Should Not Exist    ${SOPHOS_INSTALL}
    Run Default Thininstaller  expected_return_code=${18}
    Should Not Exist    ${SOPHOS_INSTALL}

    Check Thininstaller Log Contains    ERROR Installation of ServerProtectionLinux-Base-component failed.
    Check Thininstaller Log Contains    ERROR ServerProtectionLinux-Base-component: THIS IS BAD
    Check Thininstaller Log Contains    ERROR See ServerProtectionLinux-Base-component_install.log for the full installer output.
    Check Thininstaller Log Contains    ERROR Base installation failed, not installing any plugins
    Check Thininstaller Log Contains    ERROR Failed to start product
    Check Thininstaller Log Contains    Failed to install at least one of the packages, see logs for details
    Check Thininstaller Log Contains    Copying SPL logs to
    Check Thininstaller Log Contains    Removing /opt/sophos-spl
    Check Thininstaller Log Contains    Please see https://support.sophos.com/support/s/article/KB-000041952 for troubleshooting help
