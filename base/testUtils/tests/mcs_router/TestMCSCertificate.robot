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
    Setup MCS CA With Incorrect Permissions  ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem
    Register With Local Cloud Server
    Check Mcsrouter Log Contains   Unable to load CA certificates from '/tmp/tempcertdir/root-ca.crt.pem' as it isn't a file

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Log Contains   ${SOPHOS_INSTALL}/base/bin/mcsrouter died with 100   ${SOPHOS_INSTALL}/logs/base/watchdog.log   Watchdog

Successful Register With Central Replacing MCS Certificate
    Copy File   ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem  /tmp/mcs_rootca.crt.0
    Copy CA To File   /tmp/mcs_rootca.crt.0  ${SOPHOS_INSTALL}/base/mcs/certs/
    File Should Not Exist   ${SOPHOS_INSTALL}/base/mcs/policy/MCS-25_policy.xml
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running
    Wait New MCS Policy Downloaded
