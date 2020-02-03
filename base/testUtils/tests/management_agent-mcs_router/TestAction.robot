*** Settings ***
Library    OperatingSystem
Library    ${libs_directory}/MCSRouter.py
Library    ${libs_directory}/FakePluginWrapper.py
Library    ${libs_directory}/LogUtils.py

Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../mcs_router/McsRouterResources.robot

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
    # clean up
    Stop Plugin
    Stop Management Agent


*** Keywords ***
Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}


