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

    ${LiveQueryPath} =  Set Variable  ${SOPHOS_INSTALL}/base/mcs/action/LiveQuery_firstcommand_FakeTime_request.json
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  File should exist  ${LiveQueryPath}

    Verify LiveQuery Request Has The Expected Fields  ${LiveQueryPath}  type=sophos.mgt.action.RunLiveQuery  name=Test Query Special   query=select * from process

    Trigger Update Now
    Send Query From Fake Cloud    Test Another Special   select * from process   command_id=secondcommand

    #all the different actions can be processed along with livequery and end up written to disk
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Action File Exists   LiveQuery_secondcommand_FakeTime_request.json
    Check Action File Exists    LiveQuery_firstcommand_FakeTime_request.json
    Check Action File Exists    ALC_action_FakeTime.xml
