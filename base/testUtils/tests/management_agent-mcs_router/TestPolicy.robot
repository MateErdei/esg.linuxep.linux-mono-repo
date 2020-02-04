*** Settings ***
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot

*** Test Case ***

Default SAV Policy Is Written To File and Passed Through The Management Agent To The Plugin
    [Tags]  MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Setup Plugin Registry
    Start Management Agent
    Start Plugin

    Start MCSRouter
    Check Default Policies Exist

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Policy

    ${pluginPolicy} =    Get Plugin Policy

    ${pluginPolicyFromFile} =    Get File    ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml

    Should Be True   """${pluginPolicy}""" == """${pluginPolicyFromFile}"""

    # clean up
    Stop Plugin
    Stop Management Agent
    Check Temp Policy Folder Doesnt Contain Policies

