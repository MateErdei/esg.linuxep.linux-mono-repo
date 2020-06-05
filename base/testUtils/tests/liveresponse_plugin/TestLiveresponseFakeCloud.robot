*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LiveResponseUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${LIBS_DIRECTORY}/WebsocketWrapper.py

Library     String
Library     OperatingSystem
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../mcs_router/McsPushClientResources.robot

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../watchdog/WatchdogResources.robot

Resource  LiveresponseResources.robot
Resource  ../upgrade_product/UpgradeResources.robot


Test Setup  Liveresponse Test Setup
Test Teardown  Liveresponse Test Teardown

Suite Setup   Liveresponse Suite Setup
Suite Teardown   Liveresponse Suite Teardown

Default Tags   LIVERESPONSE_PLUGIN  MANAGEMENT_AGENT

*** Variables ***
${Thumbprint}               2D03C43BFC9EBE133E0C22DF61B840F94B3A3BD5B05D1D715CC92B2DEBCB6F9D
${websocket_server_url}     wss://localhost.com

*** Test Cases ***
Liveresponse Can Start
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Liveresponse Log Contains  liveresponse <> Entering the main loop

    Start Websocket Server

    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${path} =  Set Variable  end_to_end_test
    Check Liveresponse Command Successfully Starts A Session   ${path}

    Send Message With Newline   systemctl status sophos-spl   ${path}
    Match Message   sophos-live-terminal   ${path}

*** Keywords ***
Liveresponse Suite Setup
    Start MCS Push Server
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Set Local CA Environment Variable
    Require Fresh Install
    create file  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server
    Override LogConf File as Global Level  DEBUG

    Wait New MCS Policy Downloaded
    Install Liveresponse Directly

Liveresponse Suite Teardown
    Uninstall SSPL

Liveresponse Test Setup
    Require Installed
    Restart Liveresponse Plugin  True
    Wait Keyword Succeed  Liveresponse Plugin Is Running

Liveresponse Test Teardown
    Stop Websocket Server
    ${files} =  List Directory   ${MCS_DIR}/action/
    Push Server Teardown with MCS Fake Server
    Stop Proxy Servers
#    Run Keyword If Test Failed  LogUtils.Dump Log  ${WEBSPCKET_SERVER_LOG}

Check Liveresponse Command Successfully Starts A Session
    [Arguments]   ${session_path}
    Mark Managementagent Log
    ${liveResponse} =  Create Live Response Action  ${websocket_server_url}/${session_path}  ${Thumbprint}
    Send Message To Push Server   ${liveResponse}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command from Push Server

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Marked Managementagent Log Contains   Action /opt/sophos-spl/base/mcs/action/LiveTerminal_action_FakeTime.xml

#    Match Message   @   ${session_path}