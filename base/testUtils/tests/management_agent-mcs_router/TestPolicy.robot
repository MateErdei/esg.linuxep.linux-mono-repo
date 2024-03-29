*** Settings ***
Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/FakePluginWrapper.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Force Tags    MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_PARALLEL2

*** Test Case ***
Default SAV Policy Is Written To File and Passed Through The Management Agent To The Plugin
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
    Check Temp Policy Folder Doesnt Contain Policies

