*** Settings ***
Library    OperatingSystem
Library     Process

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Test Teardown     Test Action Teardown

Force Tags    MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_PARALLEL2

*** Test Case ***
Verify Scan Now Action Sent Through MCS Router And Management Agent Will Be Processed By Plugin
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Remove Action Xml Files
    Start MCSRouter
    Setup Plugin Registry
    Start Management Agent
    Start Plugin
    Check Cloud Server Log For Command Poll
    Trigger Scan Now
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2

    Wait Until Keyword Succeeds
    ...  25 secs
    ...  1 secs
    ...  Get Plugin Action
    ${pluginAction} =  Get Plugin Action

    Should Be True    "ScanNow" in """${pluginAction}"""

    Check Temp Folder Doesnt Contain Atomic Files


Verify Initiate LiveTerminal Action Sent Through MCS Router And Management Agent Will Be Processed By Plugin
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Remove Action Xml Files
    Start MCSRouter
    Set Fake Plugin App Id   LiveTerminal
    Setup Plugin Registry
    Start Management Agent
    Start Plugin
    Check Cloud Server Log For Command Poll

    Trigger Initiate Live Terminal
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Action
    ${pluginAction} =  Get Plugin Action

    Should Be True    "sophos.mgt.action.InitiateLiveTerminal" in """${pluginAction}"""

    Check Temp Folder Doesnt Contain Atomic Files

Verify Management Agent Handles CORE Health Reset Action When Received
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Remove Action Xml Files
    Start MCSRouter
    Setup Plugin Registry
    Start Management Agent
    Start Plugin

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes
    ${healthStatusXml} =    Get File    ${SHS_STATUS_FILE}

    ${content}    Evaluate    str('{"ThreatHealth": 2}')
    Send Plugin Threat Health   ${content}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  <item name="threat" value="2" />

    Check Cloud Server Log For Command Poll

    Mark Management Agent Log
    Mark Mcsrouter Log

    Trigger Health Reset
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains In Order
         ...  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
         ...  PluginManager: Queue action CORE
         ...  Processing Health Reset Action.

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  <item name="threat" value="1" />

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Marked Mcsrouter Log Contains String N Times  Sending status for SHS adapter  1

    Check Temp Folder Doesnt Contain Atomic Files


*** Keywords ***
Test Action Teardown
    # clean up
 	Run Keyword And Ignore Error  Stop Plugin
    Run Keyword And Ignore Error  Stop Management Agent
    Remove Fake Plugin From Registry
    MCSRouter Default Test Teardown

Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}


