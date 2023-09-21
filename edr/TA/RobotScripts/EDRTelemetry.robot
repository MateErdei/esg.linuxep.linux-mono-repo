*** Settings ***
Documentation    Testing EDR Telemetry

Library         Process
Library         OperatingSystem
Library         Collections
Library         ../Libs/XDRLibs.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     No Operation
Suite Teardown  Uninstall All

Test Setup      Run Keywords
...  Install With Base SDDS  AND
...  Check EDR Plugin Installed With Base

Test Teardown   EDR And Base Teardown

Default Tags    TAP_PARALLEL4

*** Test Cases ***
Edr Plugin reports health correctly
    [Setup]  No operation
    Install Base For Component Tests
    Install EDR Directly from SDDS
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  0
    Run Process  mv  ${SOPHOS_INSTALL}/plugins/edr/bin/osqueryd  /tmp
    Stop EDR
    Start edr
    Wait Until Keyword Succeeds
    ...   30 secs
    ...   2 secs
    ...   File Should Contain  ${EDR_LOG_PATH}   Unable to execute osquery
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  1
    ${mark} =  Mark File  ${EDR_LOG_PATH}
    Wait Until Keyword Succeeds
    ...  90 secs
    ...  5 secs
    ...  Marked File Contains  ${EDR_LOG_PATH}  The osquery process failed to start   ${mark}

    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  1
    Run Process  mv  /tmp/osqueryd  ${SOPHOS_INSTALL}/plugins/edr/bin/
    Wait Until Keyword Succeeds
    ...  45 secs
    ...  5 secs
    ...  Marked File Contains  ${EDR_LOG_PATH}  SophosExtension running  ${mark}

    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  0