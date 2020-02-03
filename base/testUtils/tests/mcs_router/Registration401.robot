*** Settings ***
Documentation    Tests to verify we can register successfully with
...              fake cloud and save the ID and password we receive.
...              Also tests bad registrations, and deregistrations.

Library     String
Library     Process

Resource  McsRouterResources.robot

Test Teardown    Registration 401 Test Teardown

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

*** Keywords ***
Registration 401 Test Teardown
    Stop 401 Server  ${serverHandle}

    MCSRouter Default Test Teardown
    stop system watchdog

*** Test Case ***
Register fails with server producing 401 errors
    [Documentation]  Derived from CLOUD.ERROR.004_Handle_401_errors.sh
    [Tags]  MCS  MCS_ROUTER
    ${port} =  Set Variable  4443
    ${serverHandle} =  Start 401 Server  ${port}
    Set Test Variable  ${serverHandle}  ${serverHandle}

    ${exitCode} =  fail_register  ThisIsAnMCSToken  https://localhost:${port}/sophos/management/ep
    Should Be Equal As Integers  ${exitCode}  6

    ${logfile} =  Get Register Central Log
    check log contains  UNAUTHORIZED from server 401  ${logfile}  register_central.log
