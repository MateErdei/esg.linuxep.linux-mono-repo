*** Settings ***
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot

*** Test Case ***

Verify Status Sent To Management Agent Will Be Passed To MCS And Received In Fake Cloud
    [Tags]  MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter

    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin


    ${statusContent}    Evaluate    str('status contents')
    Send Plugin Status   ${statusContent}

    Check Status File           ${statusContent}
    Check Status Cache File     ${statusContent}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    ${statusContent}   1

    # clean up
    Stop Plugin
    Stop Management Agent