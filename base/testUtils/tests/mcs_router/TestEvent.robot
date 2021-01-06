*** Settings ***
Library   OperatingSystem
Library   Process

Resource  McsRouterResources.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server

Suite Teardown    Run Keywords
...               Stop Local Cloud Server  AND
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

*** Test Case ***
Update Succeeded Event Sent On Change
    [Tags]  SMOKE  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_TESTS
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Update Event    0
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For Update Event    0
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  5 secs
    ...  Check Event Directory Empty

ALC event Sent On reregister
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Update Event    0
    list files in directory  /opt/sophos-spl/base/mcs/event
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For Update Event    0
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  5 secs
    ...  Check Event Directory Empty
    Send Cmd To Fake Cloud  controller/reregisterNext

    Wait EndPoint Report It Has Re-registered
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Cloud Server Log Contains Pattern    .*Register with.*PolicyRegToken.*
    Wait Until Keyword Succeeds
    ...  1 min
    ...  15 secs
    ...  Check Mcsrouter Log Contains    Endpoint re-registered
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2
Update Failure Event Sent On Start Up
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Send Update Event    107
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For Update Event    107
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  5 secs
    ...  Check Event Directory Empty


SAV Event Sent With Non Ascii Character
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Start MCSRouter

    Install FakeSavPlugin

    ${chineseevent}=  Set Variable  ${SUPPORT_FILES}/CentralXml/SAV_nonascii-event.xml
    Send Event File  SAV  ${chineseevent}
    File Should Exist  ${SOPHOS_INSTALL}/base/mcs/event/SAV_event-001.xml

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Cloud Server Log Contains   Virus File found in /tmp/  1

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Event Directory Empty
    Sleep  2 secs   Wait event to come back

    # simulate user asking to clear the notified event. Which causes central to send a
    # clear event with non ascii code.
    Trigger Clear Nonascii

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Cloud Server Log Contains   Triggering an on demand clear action  1
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Cloud Server Log Contains   SAVClearFromList  1

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Envelope Log Contains   SAVClearFromList  1

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Action File Exists   SAV_action_FakeTime.xml

    Log File  ${SOPHOS_INSTALL}/base/mcs/action/SAV_action_FakeTime.xml

Verify Large Event XML Gets Rejected

    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Event File    ALC    ${SUPPORT_FILES}/CentralXml/ALC_large_event.xml

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Refusing to parse, size of status exceeds size limit

Verify Event XML Containing Script Tag Gets Rejected

    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Event File    ALC    ${SUPPORT_FILES}/CentralXml/ALC_with_script_tag.xml

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Refusing to parse Script Element

*** Keywords ***
Install FakeSavPlugin
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   copyPluginRegistration   ${SUPPORT_FILES}/fakesavplugin.json
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0