*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/WebsocketWrapper.py
Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${LIBS_DIRECTORY}/LiveResponseUtils.py



Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot
Resource  LiveResponseResources.robot
Resource  ../upgrade_product/UpgradeResources.robot


Test Setup  LiveResponse Telemetry Test Setup
Test Teardown  LiveResponse Telemetry Test Teardown

Suite Setup   LiveResponse Telemetry Suite Setup
Suite Teardown   LiveResponse Telemetry Suite Teardown

Default Tags   LIVERESPONSE_PLUGIN

*** Variables ***
${Thumbprint}               2D03C43BFC9EBE133E0C22DF61B840F94B3A3BD5B05D1D715CC92B2DEBCB6F9D
${websocket_server_url}     wss://localhost
*** Test Cases ***
Liveresponse Plugin Proxy
    [Documentation]    Check Watchdog Telemetry When Liveresponse Is Present

    Install Live Response Directly
    Check Live Response Plugin Installed
    Start Proxy Server With Basic Auth    3000    username   password
    Set Environment Variable  https_proxy   http://username:password@localhost:3000
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Exist  /opt/sophos-spl/base/etc/sophosspl/current_proxy
    Log File  /opt/sophos-spl/base/etc/sophosspl/current_proxy
    ${correlationId} =  Get Correlation Id

    Mark Managementagent Log
    ${liveResponse} =  create_live_response_action_fake_cloud  ${websocket_server_url}  ${Thumbprint}
    run_live_response  ${liveResponse}   ${correlationId}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...   Check Marked Managementagent Log Contains   Action /opt/sophos-spl/base/mcs/action/LiveTerminal_${correlationId}_action_
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Sessions Log Contains   Using proxy: localhost:3000
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Sessions Log Contains   Connected to subscriber, url: wss://localhost



*** Keywords ***
LiveResponse Telemetry Suite Setup
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Set Local CA Environment Variable
    Install LT Server Certificates

    Start Local Cloud Server



LiveResponse Telemetry Suite Teardown
    Uninstall SSPL

LiveResponse Telemetry Test Setup
    Require Installed
    Start Websocket Server


LiveResponse Telemetry Test Teardown
    General Test Teardown
    Restart Liveresponse Plugin  True
    Stop Websocket Server
    ${files} =  List Directory   ${MCS_DIR}/action/
    ${liveterminal_server_log} =  Liveterminal Server Log File
    Log File  ${liveterminal_server_log}

