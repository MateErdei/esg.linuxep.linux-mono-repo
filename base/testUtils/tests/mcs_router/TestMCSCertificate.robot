*** Settings ***
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Test Setup  Setup
Suite Teardown  Remove Directory  /tmp/tempcertdir   True
Test Teardown  Run Keywords
...            Unset CA Environment Variable   AND
...            Stop Local Cloud Server  AND
...            MCSRouter Default Test Teardown

Force Tags   MCS_ROUTER  TAP_PARALLEL4

*** Keywords ***
Setup
    Require Fresh Install
    Start Local Cloud Server

*** Test Cases ***
MCS Router Stops If MCS Certificate Cannot Be Read By Sophos-spl-user
    Setup MCS CA With Incorrect Permissions  ${COMMON_TEST_UTILS}/server_certs/server-root.crt
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Unable to load CA certificates from '/tmp/tempcertdir/server-root.crt' as it isn't a file

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Log Contains   ${SOPHOS_INSTALL}/base/bin/mcsrouter died with exit code 100   ${SOPHOS_INSTALL}/logs/base/watchdog.log   Watchdog

Successful Register With Central Replacing MCS Certificate
    Copy File   ${COMMON_TEST_UTILS}/server_certs/server-root.crt  /tmp/mcs_rootca.crt.0
    Copy CA To File   /tmp/mcs_rootca.crt.0  ${SOPHOS_INSTALL}/base/mcs/certs/
    File Should Not Exist   ${SOPHOS_INSTALL}/base/mcs/policy/MCS-25_policy.xml
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running
    Wait New MCS Policy Downloaded
