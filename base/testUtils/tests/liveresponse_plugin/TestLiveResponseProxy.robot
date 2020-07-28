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


Test Setup  LiveResponse Test Setup
Test Teardown  LiveResponse Test Teardown

Suite Setup   LiveResponse Suite Setup
Suite Teardown   LiveResponse Suite Teardown

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
    Wait Until Keyword Succeeds
        ...  10
        ...  1
        ...  File Should Exist    ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    ${correlationId} =  Get Correlation Id

    Mark Managementagent Log
    ${liveResponse} =  Create Live Response Action Fake Cloud  ${websocket_server_url}  ${Thumbprint}
    Run Live Response  ${liveResponse}   ${correlationId}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...   Check Marked Managementagent Log Contains   Action LiveTerminal_${correlationId}_action_
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Sessions Log Contains   Using proxy: localhost:3000
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Sessions Log Contains   Connected to subscriber, url: wss://localhost

