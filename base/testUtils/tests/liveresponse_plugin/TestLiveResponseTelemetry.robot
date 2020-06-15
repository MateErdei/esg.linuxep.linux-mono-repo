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


Liveresponse Plugin Session Count Defaults To Zero
    [Documentation]    Check session count telemetry defaults to zero when when no liveresponse sessions are run.
    Wait Until Keyword Succeeds
    ...  40s
    ...  5s
    ...  Check Expected Base Processes Are Running
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}
    ${sessions} =  Set Variable  0
    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  ${sessions}

Liveresponse Plugin Session Count
    [Documentation]    Check session count telemetry is correct when liveresponse sessions are run.

    Wait Until Keyword Succeeds
    ...  40s
    ...  5s
    ...  Check Expected Base Processes Are Running

    # Write Action file.
    ${actionTempName} =    Set Variable   /tmp/temp_liveresponse_action.xml
    ${actionContents} =    Set Variable   <action type="sophos.mgt.action.InitiateLiveTerminal"><url>url</url><thumbprint>thumbprint</thumbprint></action>
    ${actionFileName1} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/LiveTerminal_action_1_2592240006
    ${actionFileName2} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/LiveTerminal_action_2_2592240006

    # Create 1st action
    Create File     ${actionTempName}   ${actionContents}
    Move File       ${actionTempName}   ${actionFileName1}

    # Create 2nd action
    Create File     ${actionTempName}   ${actionContents}
    Move File       ${actionTempName}   ${actionFileName2}

    # Wait for two sessions to have been started
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${LIVERESPONSE_DIR}/log/liveresponse.log   liveresponse.log   Session   2

    # Run telemetry
    Prepare To Run Telemetry Executable
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}

    # Expect 2 sessions to have been run and reported in telemetry
    ${sessions} =  Set Variable  2
    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  ${sessions}


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
