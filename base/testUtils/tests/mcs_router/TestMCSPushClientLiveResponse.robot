*** Settings ***

Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/LiveResponseUtils.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py

Library     String
Resource    McsRouterResources.robot
Resource    McsPushClientResources.robot
Resource    ../upgrade_product/UpgradeResources.robot

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Test Teardown    Live Response Test Teardown

Default Tags  FAKE_CLOUD  MCS  MCS_ROUTER   TAP_TESTS
Force Tags  LOAD3


*** Test Case ***
MCSRouter Can Start and Receive LiveTerminal Action Via Push Client
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Set Fake Plugin App ID   LiveTerminal
    Setup Plugin Registry

    Check Connected To Fake Cloud

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${liveResponse} =  Create Live Response Action  wss://test-url  FakeThumbprint
    Send Message To Push Server   ${liveResponse}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command from Push Server

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Only 1 Action File Present

    ${Files} =  List Files In Directory  ${MCS_DIR}/action/
    Should Contain    ${Files[0]}   LiveTerminal_correlation-id_action

    ${content} =  Get File  ${MCS_DIR}/action/${Files[0]}
    Should Contain  ${content}  <action type="sophos.mgt.action.InitiateLiveTerminal">
    Should Contain  ${content}  wss://test-url


*** Keywords ***
Live Response Test Teardown
    Push Client Test Teardown
    Remove Fake Plugin From Registry

Only 1 Action File Present
    ${Files} =  List Files In Directory  ${MCS_DIR}/action/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1