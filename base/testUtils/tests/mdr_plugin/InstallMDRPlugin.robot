*** Settings ***
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/MDRUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../mcs_router-nova/McsRouterNovaResources.robot
Resource  ../installer/InstallerResources.robot
Resource  MDRResources.robot
Resource  ../management_agent/ManagementAgentResources.robot
Resource  ../GeneralTeardownResource.robot


Suite Setup     Require Fresh Install

Suite Teardown  Require Uninstalled

Test Setup     MDR Test Setup
Test Teardown  Test Teardown

Default Tags   MDR_PLUGIN


*** Test Cases ***

MDR Plugin Installs
    [Tags]  SMOKE  MDR_PLUGIN
    Install MDR Directly
    Check MDR Plugin Installed
    File Exists With Permissions      ${MDR_LOG_FILE}     root  root              -rw-------
    Socket Exists With Permissions    ${IPC_FILE}         root  sophos-spl-group  srw-rw----
    Directory Should Exist  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/logs

MDR Plugin Installs With Version Ini File
    Install MDR Directly
    Check MDR Plugin Installed
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini   Sophos Managed Threat Response plug-in

MDR Installer Detects Stage1 Dark Bytes In Default Location And Removes It
    Install Fake Stage1 Dark Bytes
    Directory Should Exist  /opt/dbhs
    Install MDR Directly
    Check MDR Plugin Installed
    Directory Should Not Exist  /opt/dbhs

MDR Uninstaller Does Not Report That It Could Not Remove MDR If Watchdog Is Not Running
    [Teardown]  Restart Watchdog and MDR Test Teardown
    Install MDR Directly
    Check MDR Plugin Installed
    ${systemctlResult} =  Run Process   systemctl stop sophos-spl   shell=yes
    Check Watchdog Not Running
    Should Be Equal As Strings  ${systemctlResult.rc}  0
    ${uninstallResult} =  Uninstall MDR Plugin And Return Result
    Should Not Contain  ${uninstallResult.stderr}  Failed to remove mtr: Watchdog is not running

MDR Removes Ipc And Status Files When Uninstalled
    Install MDR Directly
    Stop Management Agent Via WDCTL
    Start Management Agent Via WDCTL
    Wait for MDR Status

    # ${IPC_FILE} is a socket, and therefore is not picked up by "File Should Exist"
    Should Exist            ${IPC_FILE}
    File Should Exist       ${MDR_STATUS_XML}
    File Should Exist       ${CACHED_STATUS_XML}

    Uninstall MDR Plugin

    # Similarly, "File Should Not Exist" will always pass on ${IPC_FILE}
    Should Not Exist        ${IPC_FILE}
    File Should Not Exist   ${MDR_STATUS_XML}
    File Should Not Exist   ${CACHED_STATUS_XML}

*** Keywords ***

MDR Test Setup
    Block Connection Between EndPoint And FleetManager


Test Teardown
    MDR Test Teardown
    Remove Dir If Exists   /opt/dbhs


Restart Watchdog and MDR Test Teardown
    Require Watchdog Running
    MDR Test Teardown