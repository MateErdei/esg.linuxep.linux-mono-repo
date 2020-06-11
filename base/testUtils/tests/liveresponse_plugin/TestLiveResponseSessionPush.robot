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
Resource    ../management_agent/ManagementAgentResources.robot
Resource  LiveResponseResources.robot


Test Setup  Liveresponse Test Setup
Test Teardown  Liveresponse Test Teardown

Suite Setup   Liveresponse Suite Setup
Suite Teardown   Liveresponse Suite Teardown

Default Tags   LIVERESPONSE_PLUGIN  MANAGEMENT_AGENT  MCSROUTER  SMOKE

*** Variables ***
${Thumbprint}               2D03C43BFC9EBE133E0C22DF61B840F94B3A3BD5B05D1D715CC92B2DEBCB6F9D
${websocket_server_url}     wss://localhost

*** Test Cases ***
Verify Liveresponse Works End To End LiveResponse Session Command Via Push
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${random_id} =  Get Correlation Id
    ${path} =  Set Variable  /${random_id}
    ${tmp_test_filepath} =  Set Variable  /tmp/test_${random_id}.txt

    Check Liveresponse Command Successfully Starts A Session   ${path}

    Send Message With Newline   systemctl status sophos-spl   ${path}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Match Message   sophos-live-terminal   ${path}

    Check Touch Creates Files Successfully From Liveresponse Session   ${tmp_test_filepath}   ${path}

    Check Liveresponse Session Will Stop When Instructed by Central   ${path}


*** Keywords ***
Check Liveresponse Command Successfully Starts A Session
    [Arguments]   ${session_path}
    Mark Managementagent Log
    ${liveResponse} =  Create Live Response Action  ${websocket_server_url}${session_path}  ${Thumbprint}
    Send Message To Push Server   ${liveResponse}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command from Push Server
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...   Check Marked Managementagent Log Contains   Action /opt/sophos-spl/base/mcs/action/LiveTerminal_action_FakeTime.xml
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Match Message   root@   ${session_path}

Check Touch Creates Files Successfully From Liveresponse Session
    [Arguments]  ${test_filepath}   ${path}
    Send Message With Newline   touch ${test_filepath}   ${path}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${test_filepath}
    Remove File   ${test_filepath}

Check Liveresponse Session Will Stop When Instructed by Central
    [Arguments]  ${session_path}
    Send Message With Newline   exit   ${session_path}
    Check Liveresponse Agent Executable is Not Running

Liveresponse Test Setup
    Require Installed
    Start Websocket Server
    Restart Liveresponse Plugin  True
    Check Live Response Plugin Installed

Liveresponse Test Teardown
    Stop Websocket Server
    ${files} =  List Directory   ${MCS_DIR}/action/
    Push Server Teardown with MCS Fake Server
    Stop Proxy Servers
    ${liveterminal_server_log} =  Liveterminal Server Log File
    Log File  ${liveterminal_server_log}


Liveresponse Suite Setup
    Generate Local Fake Cloud Certificates
    Setup Suite Tmp Dir   ./tmp
    Setup Base FakeCloud And FakeCentral-LT Servers
    Install Live Response Directly

Liveresponse Suite Teardown
    Uninstall SSPL
    uninstall LT Server Certificates

Setup Base FakeCloud And FakeCentral-LT Servers
    Install LT Server Certificates
    Start MCS Push Server
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Set Local CA Environment Variable

    Require Fresh Install
    create file  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server
    Override LogConf File as Global Level  DEBUG
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log   liveresponse   DEBUG