*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py

Resource    ManagementAgentResources.robot

Test Teardown     Plugin Action Test Teardown


*** Test Cases ***
Verify Management Agent Sends Action When Action Received
    [Tags]    MANAGEMENT_AGENT  SMOKE

    # make sure no previous event xml file exists.
    Remove Action Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    # Write Action file.
    ${actionFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/SAV_action_timestamp.xml
    ${actionTmpName} =    Set Variable   ${SOPHOS_INSTALL}/base/mcs/action/TMP-SAV_action_timestamp.xml
    ${actionContents} =    Set Variable   action1
    Create File     ${actionTmpName}   ${actionContents}
    Move File       ${actionTmpName}   ${actionFileName}
    
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Action
    ${pluginAction} =  Get Plugin Action

    Should Be True   """${pluginAction}""" == """${actionContents}"""


Verify Management Agent Sends LiveQuery To EDR Plugin
    [Tags]    MANAGEMENT_AGENT  EDR_PLUGIN
    Set Fake Plugin App Id   LiveQuery
    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    # Write Action file.
    ${actionFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/LiveQuery_correlation-id_FakeTime_request.json

    ${actionTmpName} =    Set Variable   ${SOPHOS_INSTALL}/tmp/TempAction.json
    ${liveQueryContent} =    Set Variable   Live Query Content
    Create File     ${actionTmpName}   ${liveQueryContent}
    Move File       ${actionTmpName}   ${actionFileName}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Action
    ${pluginAction} =  Get Plugin Action

    Should Be True   """${pluginAction}""" == """${liveQueryContent}"""
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains In Order
         ...  tmp/fake_plugin.log
         ...  APPID=LiveQuery
         ...  CorrelationId=correlation-id


*** Keywords ***

Plugin Action Test Teardown
    Run Keyword If Test Failed     Log File   tmp/fake_plugin.log
    General Test Teardown
    Stop Plugin
    Stop Management Agent
    Terminate All Processes  kill=True

