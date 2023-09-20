*** Settings ***
Documentation    Tests to verify we can register successfully with
...              fake cloud with C++ implementation.

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Resource  ../installer/InstallerResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Test Setup       Run Keywords
...              Start Local Cloud Server

Test Teardown    Run Keywords
...              Stop Local Cloud Server  AND
...			     Stop System Watchdog

Default Tags  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER
Force Tags  LOAD3

*** Test Case ***
Successful Registration In C
    [Tags]  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER  TAP_PARALLEL1
    ${result} =  Run Process  /opt/sophos-spl/base/bin/centralregistration  ThisIsARegToken  https://localhost:4443/mcs
    Log  ${result.stdout}
    Log  ${result.stderr}
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    Should Contain  ${result.stderr}  Product successfully registered

Successful Registration In C With Group
    [Tags]  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER  TAP_PARALLEL4
    ${result} =  Run Process  /opt/sophos-spl/base/bin/centralregistration  ThisIsARegToken  https://localhost:4443/mcs  --central-group\=ctestgroup
    Log  ${result.stdout}
    Log  ${result.stderr}
    Check Cloud Server Log Contains  <deviceGroup>ctestgroup</deviceGroup>
    Should Contain  ${result.stderr}  Product successfully registered

Message Relay Prioritisation
    [Tags]  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER  TAP_PARALLEL4
    ${result} =  Run Process  /opt/sophos-spl/base/bin/centralregistration
        ...  ThisIsARegToken
        ...  https://localhost:4443/mcs
        ...  --message-relay  localhost:10000,1,relay1;localhost:20000,0,relay2
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${log_location} =  Set Variable  /tmp/TestCRegistration.stderr
    Create File  ${log_location}  ${result.stderr}
    Check Log Contains In Order  ${log_location}
    ...  Trying 127.0.0.1:20000
    ...  Trying 127.0.0.1:10000
    Remove File  ${log_location}
    Should Contain  ${result.stderr}  Product successfully registered