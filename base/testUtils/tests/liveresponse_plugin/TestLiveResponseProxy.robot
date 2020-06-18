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


*** Test Cases ***
Liveresponse Plugin Proxy
    [Documentation]    Check Watchdog Telemetry When Liveresponse Is Present

    Install Live Response Directly
    Start Proxy Server With Basic Auth    3000    username   password
    Set Environment Variable  https_proxy   http://username:password@localhost:3000
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Exist  /opt/sophos-spl/base/etc/sophosspl/current_proxy
    Log File  /opt/sophos-spl/base/etc/sophosspl/current_proxy
    ${random_id} =  Get Correlation Id
    ${path} =  Set Variable  /${random_id}
    ${tmp_test_filepath} =  Set Variable  /tmp/test_${random_id}.txt

    Check Liveresponse Command Successfully Starts A Session   ${path}





*** Keywords ***
LiveResponse Telemetry Suite Setup
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Set Local CA Environment Variable
    Install LT Server Certificates
    Start MCS Push Server
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml


LiveResponse Telemetry Suite Teardown
    Uninstall SSPL

LiveResponse Telemetry Test Setup
    Require Installed


LiveResponse Telemetry Test Teardown
    Restart Liveresponse Plugin  True
    General Test Teardown

