*** Settings ***
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${LIBS_DIRECTORY}/LiveQueryUtils.py

Resource  McsRouterResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server
Suite Teardown    Run Keywords
...               Stop Local Cloud Server  AND
...               Uninstall SSPL Unless Cleanup Disabled


Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_TESTS

*** Test Case ***
Query can be Sent From Fake Cloud and Received by MCS Router
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Set Fake Plugin App ID   LiveQuery
    Setup Plugin Registry

    Start MCSRouter

    Send Query From Fake Cloud    Test Query Special   select * from process   command_id=firstcommand
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains   /commands/applications/MCS;ALC;LiveQuery

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains       Test Query Special

    ${files} =  List Files In Directory  ${SOPHOS_INSTALL}/base/mcs/action/
    ${LiveQueryPath} =  Set Variable  ${SOPHOS_INSTALL}/base/mcs/action/${files[0]}

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File should exist  ${LiveQueryPath}

    Verify LiveQuery Request Has The Expected Fields  ${LiveQueryPath}  type=sophos.mgt.action.RunLiveQuery  name=Test Query Special   query=select * from process

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Action File Exists    ALC_action_FakeTime.xml

    Send Query From Fake Cloud    Test Another Special   select * from process   command_id=secondcommand

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check 3 Action Files Exist

    ${files} =  List Files In Directory  ${SOPHOS_INSTALL}/base/mcs/action/
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Action File Exists   ${files[2]}

*** Keywords ***
Check 3 Action Files Exist
    ${count} =  Count Files In Directory  ${SOPHOS_INSTALL}/base/mcs/action/
    Should Be Equal As Integers  ${count}  3
