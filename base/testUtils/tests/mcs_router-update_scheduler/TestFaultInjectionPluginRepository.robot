*** Settings ***
Documentation    Fault injection cases involving corruption of the pluginRegistry folder

Library     OperatingSystem
Library     String
Library     ${LIBS_DIRECTORY}/FaultInjectionTools.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource  ../telemetry/TelemetryResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Suite Teardown   Require Uninstalled

Test Setup     Install And Wait System Stable
Test Teardown  Local Test Teardown

Default Tags  FAKE_CLOUD  MCS_ROUTER  MANAGEMENT_AGENT  TELEMETRY  WATCHDOG  FAULTINJECTION

*** Variables ***
${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml


*** Test Case ***
Plugin Entry That Can Not Be Read Should Not Break Other Plugins
    Create Directory   ${SOPHOS_INSTALL}/base/pluginRegistry/plugin.json
    Start System Watchdog
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    # directory will be ignored for the other executables reading pluginRegistry
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check MCSRouter log contains   Failed to load plugin file
    Sophos Processes Related To Plugin Registry Should be Running Normally

Invalid Plugin Entry Not Break Other Plugins
    Create File  ${SOPHOS_INSTALL}/base/pluginRegistry/plugin.json    this is an invalid json and hence invalid plugin
    Start System Watchdog
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check MCS Router log contains   File not valid json
    Sophos Processes Related To Plugin Registry Should be Running Normally
    Check Log Contains    Failed to load plugin     ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log    SophosManagement

Plugin With Binary Entry Should Not Break Other Plugins
    Copy File   /bin/echo   ${SOPHOS_INSTALL}/base/pluginRegistry/plugin.json
    Start System Watchdog
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check MCS Router log contains   File not valid json
    Check MCS Router log contains   codec can't decode
    Sophos Processes Related To Plugin Registry Should be Running Normally
    Check Log Contains    Failed to load plugin     ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log    SophosManagement

Plugin With Wrong Permissions Should Not Break Other Plugins
    Run Process   chmod  0100   ${SOPHOS_INSTALL}/base/pluginRegistry/updatescheduler.json
    Run Process   chown  root   ${SOPHOS_INSTALL}/base/pluginRegistry/updatescheduler.json
    Start System Watchdog
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check MCS Router log contains   Failed to load plugin file
    Sophos Processes Related To Plugin Registry Should be Running Normally
    Check Log Contains    Failed to load plugin     ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log    SophosManagement

Installation Without PluginRegistry Will Fail But Should Not SegFault Or Not Be Handled
    Remove Directory  ${SOPHOS_INSTALL}/base/pluginRegistry   recursive=True
    Create File      ${SOPHOS_INSTALL}/base/pluginRegistry   not a directory
    Run Process And Expect Failure    ${SOPHOS_INSTALL}/base/bin/sophos_watchdog
    Run Process And Expect Failure    ${SOPHOS_INSTALL}/base/bin/sophos_managementagent
    Run Process And Expect Failure    ${SOPHOS_INSTALL}/base/bin/tscheduler
    Run Process And Expect Failure    ${SOPHOS_INSTALL}/base/bin/mcsrouter
    Run Telemetry Executable   ${EXE_CONFIG_FILE}   ${FAILED}   checkResult=0

Remove Permission to Read Status and Write Policy From McsRouter Will Make it To Fail But Should Not Crash
    Run Process   chown  root:root   ${SOPHOS_INSTALL}/base/mcs/policy  ${SOPHOS_INSTALL}/base/mcs/status
    Start System Watchdog
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Log Contains  Failed to configure Configure Management Agent    ${SOPHOS_INSTALL}/logs/base/watchdog.log    SophosManagement


*** Keywords ***
Run Process And Expect Failure
    [Arguments]    ${execPath}
    ${result} =    Run Process   ${execPath}
    Should Not Be Equal As Integers    ${result.rc}    0   msg="${execPath} did not report failure. stdout:${result.stdout}\n err: ${result.stderr}"
    Log  ${result.stdout}
    Should Not Contain   ${result.stderr}  Critical unhandled

Status File Contain RevId
    ${fileContent} =   Get File  ${statusPath}
    Should Contain   ${fileContent}    5514d7970027ac81cc3686799c9359043dafbc72d4b809490ca82bacc4bf5026

Wait For Update Status File To Contain RevId
    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Status File Contain RevId

Install And Wait System Stable
    Require Fresh Install
    Set Local CA Environment Variable
    Start Local Cloud Server
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Wait For Update Status File To Contain RevId
    Prepare To Run Telemetry Executable
    Stop System Watchdog

Local Test Teardown
    Require No Unhandled Exception
    Log SystemCtl Update Status
    MCSRouter Default Test Teardown
    Stop Local Cloud Server
    Cleanup Telemetry Server
