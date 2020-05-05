*** Settings ***
Library    OperatingSystem
Library     Process

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../mcs_router/McsRouterResources.robot

Test Teardown     Test Action Teardown

*** Test Case ***
Verify Scan Now Action Sent Through MCS Router And Management Agent Will Be Processed By Plugin
    [Tags]  MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER
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
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Action
    ${pluginAction} =  Get Plugin Action

    Should Be True    "ScanNow" in """${pluginAction}"""

    Check Temp Folder Doesnt Contain Atomic Files


Verify Initiate LiveTerminal Action Sent Through MCS Router And Management Agent Will Be Processed By Plugin
    [Tags]  MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER
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


