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
Suite Teardown   Liveresponse Suite Teardown

Default Tags   LIVERESPONSE_PLUGIN  MANAGEMENT_AGENT  TELEMETRY

*** Variables ***
${terminal_binary}   ${LIVERESPONSE_DIR}/bin/sophos-live-terminal
*** Test Cases ***
Liveresponse Plugin Unexpected Restart Telemetry Is Reported Correctly
    [Documentation]    Check Watchdog Telemetry When Liveresponse Is Present

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


Liveresponse Plugin Session Counts Default To Zero
    [Documentation]    Check session count telemetry defaults to zero when when no liveresponse sessions are run.
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}
    ${sessions} =  Set Variable  0
    ${successful_sessions} =  Set Variable  0
    ${failed_sessions} =  Set Variable  0
    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  ${sessions}   ${failed_sessions}

Liveresponse Plugin Session Counts Failed
    [Documentation]    Check session counts telemetry is correct when liveresponse sessions are run.

    ${actionContents} =    Set Variable   <action type="sophos.mgt.action.InitiateLiveTerminal"><url>url</url><thumbprint>thumbprint</thumbprint></action>
    Write Action file   ${actionContents}

    # Wait for a session to be started
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${LIVERESPONSE_DIR}/log/liveresponse.log   liveresponse.log   Session  1

    Swap Out Real Terminal With One That Always Returns Success
    # Restoring is done in teardown if it's needed.

    # Create and trigger 2nd action
    Write Action file   ${actionContents}

    # Wait for two sessions to have been started
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${LIVERESPONSE_DIR}/log/liveresponse.log   liveresponse.log   Session  2

    # Run telemetry
    Prepare To Run Telemetry Executable
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}

    # Expect 2 sessions to have been run and reported in telemetry
    ${sessions} =  Set Variable  2
    ${failed_sessions} =  Set Variable  1
    @{list}=  Create List   url:thumbprint  2
    @{keys}=  Create List   ${list}
    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  ${sessions}  ${failed_sessions}  ${keys}

Liveresponse Plugin no session telemetry if url is empty

    ${actionContents} =    Set Variable   <action type="sophos.mgt.action.InitiateLiveTerminal"><url></url><thumbprint>thumbprint</thumbprint></action>
    Write Action file   ${actionContents}

    # Wait for a session to be started
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Liveresponse Log Contains  Failed to find url and/or thumbprint in actionXml

    Prepare To Run Telemetry Executable
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}

    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  1   1

Liveresponse Plugin no session telemetry if thumbprint is empty

    ${actionContents} =    Set Variable   <action type="sophos.mgt.action.InitiateLiveTerminal"><url>url</url><thumbprint></thumbprint></action>
    Write Action file   ${actionContents}

    # Wait for a session to be started
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Liveresponse Log Contains   Failed to find url and/or thumbprint in actionXml

    Prepare To Run Telemetry Executable
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}

    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  1   1

Liveresponse Plugin saves session telemetry if plugin stops
    Swap Out Real Terminal With One That Always Returns Success
    ${actionContents} =    Set Variable   <action type="sophos.mgt.action.InitiateLiveTerminal"><url>url</url><thumbprint>thumbprint</thumbprint></action>

    Write Action file   ${actionContents}

    # Wait for a session to be started
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Liveresponse Log Contains  Session

    Restart Liveresponse Plugin
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Live Response Plugin Running
    Prepare To Run Telemetry Executable
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}

    ${sessions} =  Set Variable  1
    ${failed_sessions} =  Set Variable  0
    @{list}=  Create List   url:thumbprint  1
    @{keys}=  Create List   ${list}
    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  ${sessions}  ${failed_sessions}  ${keys}

Liveresponse Plugin saves mutiple session with different thumbprints in telemetry
    Swap Out Real Terminal With One That Always Returns Success
    ${actionContents1} =    Set Variable   <action type="sophos.mgt.action.InitiateLiveTerminal"><url>url</url><thumbprint>thumbprint</thumbprint></action>
    ${actionContents2} =    Set Variable   <action type="sophos.mgt.action.InitiateLiveTerminal"><url>url</url><thumbprint>thumbprint2</thumbprint></action>

    Write Action file   ${actionContents1}

    # Wait for a session to be started
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Liveresponse Log Contains  Session

    Write Action file   ${actionContents2}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${LIVERESPONSE_DIR}/log/liveresponse.log   liveresponse.log   Session  2

    Prepare To Run Telemetry Executable
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}

    ${sessions} =  Set Variable  2
    ${failed_sessions} =  Set Variable  0
    @{list}=  Create List   url:thumbprint  1
    @{list2}=  Create List   url:thumbprint2  1
    @{keys}=  Create List   ${list}  ${list2}
    Check Liveresponse Telemetry Json Is Correct  ${telemetryFileContents}  ${sessions}  ${failed_sessions}  ${keys}

*** Keywords ***
LiveResponse Telemetry Suite Setup
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log   liveresponse   DEBUG
    Liveresponse Suite Setup
    Copy Telemetry Config File in To Place

LiveResponse Telemetry Test Setup
    Require Installed
    Restart Liveresponse Plugin  True
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Live Response Plugin Running

LiveResponse Telemetry Test Teardown
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    General Test Teardown
    Restore Original Live Response Terminal Binary
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove File  ${EXE_CONFIG_FILE}


Swap Out Real Terminal With One That Always Returns Success
    Move File  ${terminal_binary}  /tmp/sophos-live-terminal-original
    Create File  ${terminal_binary}  \#!/bin/bash\nsleep 0.1
    Run Process    chmod  a+x  ${terminal_binary}

Restore Original Live Response Terminal Binary
    # If needed it will restore the old live response terminal binary
    Run Keyword And Ignore Error  Move File  /tmp/sophos-live-terminal-original  ${terminal_binary}

Write Action file
    [Arguments]  ${Contents}

    ${correlation_id1} =  Get Correlation Id
    ${creation_time_and_ttl1} =  get_valid_creation_time_and_ttl
    ${actionFileName1} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/LiveTerminal_${correlation_id1}_action_${creation_time_and_ttl1}.xml

    # Create and trigger action
    Create File     /opt/temp_liveresponse_action.xml   ${Contents}
    Move File       /opt/temp_liveresponse_action.xml   ${actionFileName1}