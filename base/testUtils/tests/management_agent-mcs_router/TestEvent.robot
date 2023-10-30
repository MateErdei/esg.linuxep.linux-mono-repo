*** Settings ***
Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/FakePluginWrapper.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Force Tags    MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_PARALLEL2

*** Test Case ***
Verify Event Sent To Management Agent Will Be Passed To MCS And Received In Fake Cloud
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

