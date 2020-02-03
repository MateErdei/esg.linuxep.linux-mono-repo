*** Settings ***
Resource  McsRouterResources.robot

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled


Test Teardown    Run Keywords
...              Stop Local Cloud Server    AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

*** Test Case ***
MCS responds correctly to empty Cloud Reponse
    [Documentation]  Derived from CLOUD.ERROR.006_Empty_commands_response.sh
    Install Register And Wait First MCS Policy
    Send Command From Fake Cloud    error/null
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Mcsrouter Log Contains       Failed to parse commands
    Check MCS Router Running



MCS responds correctly to single 401
    Install Register And Wait First MCS Policy
    Send Command From Fake Cloud    controller/reregisterNext
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Mcsrouter Log Contains       Lost authentication with server
    Check MCS Router Running
