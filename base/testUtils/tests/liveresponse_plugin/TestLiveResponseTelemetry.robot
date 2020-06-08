*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py


Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  LiveResponseResources.robot
Resource  ../upgrade_product/UpgradeResources.robot
Resource  ../mdr_plugin/MDRResources.robot


Test Setup  LiveResponse Telemetry Test Setup
Test Teardown  LiveResponse Telemetry Test Teardown

Suite Setup   LiveResponse Telemetry Suite Setup
Suite Teardown   LiveResponse Telemetry Suite Teardown

Default Tags   LIVERESPONSE_PLUGIN  MANAGEMENT_AGENT  TELEMETRY


*** Test Cases ***
Liveresponse Plugin Unexpected Restart Telemetry Is Reported Correctly
    [Documentation]    Check Watchdog Telemetry When Liveresponse Is Present
    Wait Until Keyword Succeeds
    ...  30s
    ...  3s
    ...  Check Expected Base Processes Are Running

    Kill Sophos Processes That Arent Watchdog

    Wait Until Keyword Succeeds
    ...  40s
    ...  5s
    ...  Check Expected Base Processes Are Running
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}
    Check Watchdog Telemetry Json Is Correct  ${telemetryFileContents}  1  liveresponse


EDR Reports Telemetry And Stats Correctly After Plugin Restart For Live Query
    [Tags]  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY  EXCLUDE_AWS
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}


*** Keywords ***
LiveResponse Telemetry Suite Setup
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log   liveresponse   DEBUG
    Install Live Response Directly
    Copy Telemetry Config File in To Place


LiveResponse Telemetry Suite Teardown
    Uninstall SSPL

LiveResponse Telemetry Test Setup
    Require Installed
    Restart Liveresponse Plugin  True

LiveResponse Telemetry Test Teardown
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    General Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove File  ${EXE_CONFIG_FILE}
