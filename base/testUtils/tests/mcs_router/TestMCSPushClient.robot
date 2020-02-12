*** Settings ***

Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     String
Resource    McsRouterResources.robot


Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled


Test Teardown    Test Teardown

Default Tags  FAKE_CLOUD  MCS  MCS_ROUTER

*** Test Case ***

MCSRouter Can Start And Receive Messages From The Push Client
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_Push_Server.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

Push Client informs MCS Client if the connection with Push Server is disconnected and logged to MCSRouter
    Start MCS Push Server
    Configure Push Server To Ping Interval  300
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_Push_Server.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

    Wait Until Keyword Succeeds
    ...          30s
    ...          3s
    ...          Check Mcsrouter Log Contains   Push Server service reported: Error: No Ping from Server


Push Client stops connection to Push server when instructed by the MCS Client
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_Push_Server.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

    ${mcs_policy} =    Get File  ${SUPPORT_FILES}/CentralXml/MCS_policy_Push_Server.xml
    ${changed_ping} =  Replace String  ${mcs_policy}  <pushPingTimeout>10</pushPingTimeout>    <pushPingTimeout>20</pushPingTimeout>
    Send Policy   mcs  ${changed_ping}

    Wait Until Keyword Succeeds
    ...          30s
    ...          3s
    ...          Check MCS Router Log Contains In Order
    ...          MCS push client stopped
    ...          Established MCS Push Connection

Push Client Is Stopped When MCSRouter Stops
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_Push_Server.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

    Stop Mcsrouter If Running
    Wait Until Keyword Succeeds
    ...          10s
    ...          2s
    ...          Check MCS Router Log Contains
    ...          MCS push client stopped

Push Client With No Push Server
    [TAGS]  TESTFAILURE
    #LINUXDAR-839 is responsible for this story
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_Push_Server.xml


    Wait Until Keyword Succeeds
    ...          10s
    ...          1s
    ...          Run Keywords
    ...          Check Mcsrouter Log Contains   Push Server settings changed. Applying it  AND
    ...          Check Mcsrouter Log Contains   Failed to start MCS Push Client service

    # demonstrate that without intervention, it will reconnect to the server if it becomes available
    Start MCS Push Server
    Wait Until Keyword Succeeds
    ...          60s
    ...          10s
    ...          Check Mcsrouter Log Contains   Trying to re-connect to Push Server

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy

*** Keywords ***

Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Wait Until Keyword Succeeds
    ...          10s
    ...          1s
    ...          Run Keywords
    ...          Check Mcsrouter Log Contains   Push Server settings changed. Applying it    AND
    ...          Check Mcsrouter Log Contains   Established MCS Push Connection

Send Message To Push Server And Expect It In MCSRouter Log
    [Arguments]  ${message}
    Send Message To Push Server    ${message}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command: ${message}

Test Teardown
    Stop Mcsrouter If Running
    Push Server Teardown with MCS Fake Server
