*** Settings ***
Test Setup      Test Setup
Test Teardown   Test Teardown

Suite Setup      sdds3 suite setup with fakewarehouse with real base
Suite Teardown   Cleanup sdds3 Update Tests


Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     Process
Library     DateTime
Library     OperatingSystem

Resource  ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot
Resource  ../upgrade_product/UpgradeResources.robot
Resource  ThinInstallerResources.robot

Default Tags  THIN_INSTALLER
*** Keywords ***
Test Setup
    Setup HostFile
    Setup Thininstaller Test
Test Teardown
    Thininstaller Test Teardown
    Revert HostFile

Setup HostFile
    Copy File     /etc/hosts    /etc/hosts.bk
    Append To File    /etc/hosts      127.0.0.1 sustest.sophosupd.com\n
    Append To File    /etc/hosts       127.0.0.1 sdds3test.sophosupd.com\n
    Log File    /etc/hosts

Revert HostFile
    Move File    /etc/hosts.bk    /etc/hosts


*** Test Case ***
Thin Installer uses url from policy
    configure_and_run_SDDS3_thininstaller  0    force_certs_dir=${SUPPORT_FILES}/sophos_certs
    check_suldownloader_log_contains    Performing request: https://sustest.sophosupd.com:8080/v3/
    check_suldownloader_log_contains    Performing Sync using https://sdds3test.sophosupd.com:8080
