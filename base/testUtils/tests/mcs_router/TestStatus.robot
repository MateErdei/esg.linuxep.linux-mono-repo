*** Settings ***
Library    OperatingSystem

Resource  McsRouterResources.robot

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server
Suite Teardown    Run Keywords
...               Stop Local Cloud Server  AND
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

*** Test Case ***
MCS Status Sent When Message Relay Changed
    [Tags]  MCS  FAKE_CLOUD  MESSAGE_RELAY  MCS_ROUTER  SMOKE
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Start Message Relay
    Check Cloud Server Log For Command Poll
    Send MCS Policy With New Message Relay    <messageRelay priority='0' port='20000' address='localhost' id='deadbeef'/>
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For MCS Status    2

ALC Status Sent On Change
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send ALC Status    Same
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For ALC Status    Same
    Send ALC Status    Diff
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For ALC Status    Diff

ALC Status Sent On Start Up
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Send ALC Status    Same
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For ALC Status    Same

Default Statuses Sent On Start Up
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Cloud Server Log For Default Statuses

Verify Large Status XML Gets Rejected
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Status File  ALC_large_status.xml

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Refusing to parse, size of status exceeds character limit

Verify Status XML Containing Script Tag Gets Rejected
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Status File  ALC_with_script_tag.xml

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Refusing to parse Script Element


ALC Status Not Sent When Status Message Is In The Cache And Cached Timestamp Is Less Than Seven Days Old
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send ALC Status    Some ALC Status Message
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For ALC Status    Some ALC Status Message

    # re-send status message.
    Send ALC Status    Some ALC Status Message

    Sleep  60
    check_cloud_server_log_contains_pattern   .*Some ALC Status Message.*   1

ALC Status Is Sent When Status Message Is In The Cache And Cached Timestamp Is Greater Than Seven Days Old
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Override LogConf File as Global Level  DEBUG

    Start MCSRouter
    Send ALC Status    Some ALC Status Message
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For ALC Status    Some ALC Status Message

    # Drop old status_cache.json file containing ALC status older than 7 days by 1 second
    Create Back Dated Alc Status Cache File   /opt/sophos-spl/base/mcs/status/cache/status_cache.json   Some ALC Status Message

    # re-send status message.
    Send ALC Status    Some ALC Status Message

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  check_cloud_server_log_contains_pattern   .*Some ALC Status Message.*   2


ALC Status Not Sent When Status Message Is In The Cache And MCS Router Restarted
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send ALC Status    Some ALC Status Message
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For ALC Status    Some ALC Status Message

    Stop Mcsrouter If Running
    Start MCSRouter

    Sleep  60
    check_cloud_server_log_contains_pattern   .*Some ALC Status Message.*   1

ALC Status Sent When Status Message Is In The Cache And MCS Router Restarted
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Override LogConf File as Global Level  DEBUG

    Start MCSRouter
    Send ALC Status    Some ALC Status Message
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log For ALC Status    Some ALC Status Message

    # Drop old status_cache.json file containing ALC status older than 7 days by 1 second
    Create Back Dated Alc Status Cache File   /opt/sophos-spl/base/mcs/status/cache/status_cache.json   Some ALC Status Message

    Stop Mcsrouter If Running
    Start MCSRouter

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  check_cloud_server_log_contains_pattern   .*Some ALC Status Message.*   2


*** Keywords ***

Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}
