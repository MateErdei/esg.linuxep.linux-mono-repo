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

Test 401 From Central When Sending XDR Data Is Handled Correctly
    Install Register And Wait First MCS Policy
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [mcs_router]\nVERBOSITY=DEBUG\n
    Stop Mcsrouter If Running
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCS Router Running
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  Request JWT token from
    Send Command From Fake Cloud    error/serverXdr401

    Create File  /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1720641657.json  {"content" : "NotEmpty"}
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  UNAUTHORIZED from server

    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  Failed to send datafeed
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Check Mcsrouter Log Does Not Contain   Sent result, datafeed ID: scheduled_query
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log     Request JWT token from   2
    Check MCS Router Running

Test 403 From Central Is Handled Correctly
    Install Register And Wait First MCS Policy
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [mcs_router]\nVERBOSITY=DEBUG\n
    Stop Mcsrouter If Running
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCS Router Running
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  Request JWT token from
    Send Command From Fake Cloud    error/server403

    Create File  /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1720641657.json  {"content" : "NotEmpty"}
    Create File  /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1770641657.json  {"content" : "NotEmpty"}
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  HTTP Forbidden (403)

    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  Purging all datafeed files due to 403 code from Sophos Central
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Check Mcsrouter Log Does Not Contain   Sent result, datafeed ID: scheduled_query
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log     Request JWT token from   2
    Check MCS Router Running

Test 413 From Central Is Handled Correctly
    Install Register And Wait First MCS Policy
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [mcs_router]\nVERBOSITY=DEBUG\n
    Stop Mcsrouter If Running
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCS Router Running
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  Request JWT token from

    Create File  /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1730641639.json  {"content" : "NotEmpty"}
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1730641639.json

    Send Command From Fake Cloud    error/server413
    Create File  /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1730641657.json  {"content" : "NotEmpty"}
    Wait Until Keyword Succeeds
    ...  30s
    ...  2s
    ...  Check MCSRouter Log Contains  HTTP Payload Too Large (413)

    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Check MCS Router Running

Test 504 From Central Is handled correctly
    Install Register And Wait First MCS Policy
    Send Command From Fake Cloud    error/server504

    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCSRouter Log Contains  HTTP Gateway timeout (504): Gateway Timeout (b'')
    Check MCS Router Running
