*** Settings ***
Documentation    Suite description
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/WebsocketWrapper.py
Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${LIBS_DIRECTORY}/LiveResponseUtils.py

Resource  ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource  ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource  ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource  ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource  ${COMMON_TEST_ROBOT}/UpgradeResources.robot


Test Setup  LiveResponse Test Setup
Test Teardown  LiveResponse Test Teardown

Suite Setup   LiveResponse Suite Setup
Suite Teardown   LiveResponse Suite Teardown

Default Tags   LIVERESPONSE_PLUGIN

*** Variables ***
${Thumbprint}               F00ADD36235CD85DF8CA4A98DE979BDD3C48537EBEB4ADB1B31F9292105A5CA0
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
        ...  File Should Exist    ${MCS_POLICY_CONFIG}
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

