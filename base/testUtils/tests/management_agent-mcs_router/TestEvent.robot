*** Settings ***
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource    ../installer/InstallerResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../management_agent/ManagementAgentResources.robot

*** Test Case ***
Verify Event Sent To Management Agent Will Be Passed To MCS And Received In Fake Cloud
    [Tags]  MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter

    Remove Event Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    ${eventContent} =     Evaluate    str('<a>event one test</a>')
    Send Plugin Event   ${eventContent}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    ${eventContent}   1
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  5 secs
    ...  Check Event Directory Empty

    # clean up
    Stop Plugin
    Stop Management Agent
