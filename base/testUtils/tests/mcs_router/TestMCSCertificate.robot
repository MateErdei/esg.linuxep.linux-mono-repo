*** Settings ***
Resource  McsRouterResources.robot

Suite Setup  Setup
Suite Teardown  Remove Directory  /tmp/tempcertdir   True
Test Teardown  Run Keywords
...            Unset CA Environment Variable   AND
...            Stop Local Cloud Server  AND
...            MCSRouter Default Test Teardown

Default Tags   MCS_ROUTER

*** Keywords ***
Setup
    Require Fresh Install
    Start Local Cloud Server
    Regenerate Certificates

*** Test Cases ***
MCS Router Stops If MCS Certificate Cannot Be Read By Sophos-spl-user
    Setup MCS CA With Incorrect Permissions  SupportFiles/CloudAutomation/root-ca.crt.pem
    Register With Local Cloud Server
    Check Mcsrouter Log Contains   Unable to load CA certificates from '/tmp/tempcertdir/root-ca.crt.pem' as it isn't a file

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Log Contains   ${SOPHOS_INSTALL}/base/bin/mcsrouter died with 100   ${SOPHOS_INSTALL}/logs/base/watchdog.log   Watchdog
