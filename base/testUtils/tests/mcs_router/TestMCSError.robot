*** Settings ***
Resource  McsRouterResources.robot

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled


Test Teardown    Run Keywords
...              Stop Local Cloud Server    AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER
Force Tags  LOAD3

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

Test 404 From Central Is handled correctly
    Install Register And Wait First MCS Policy
    Send Command From Fake Cloud    error/server404

    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  Bad response from server 404: Not Found
    Check MCS Router Running

Test 403 From Central Is handled correctly
    Install Register And Wait First MCS Policy
    Send Command From Fake Cloud    error/server403

    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  HTTP Forbidden (403)
    Check MCS Router Running

Test 504 From Central Is handled correctly
    Install Register And Wait First MCS Policy
    Send Command From Fake Cloud    error/server504

    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  HTTP Gateway timeout (504): Gateway Timeout (b'')
    Check MCS Router Running
