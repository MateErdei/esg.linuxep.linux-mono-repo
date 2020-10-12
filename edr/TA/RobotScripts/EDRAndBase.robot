*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      No Operation
Test Teardown   EDR And Base Teardown

*** Test Cases ***
LiveQuery is Distributed to EDR Plugin and Its Answer is available to MCSRouter
    Check EDR Plugin Installed With Base
    Simulate Live Query  RequestProcesses.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_123-4_response.json

LiveQuery Response is Chowned to Sophos Spl Local on EDR Startup
    Check EDR Plugin Installed With Base
    Create File  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_567-8_response.json
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains      edr <> Plugin Finished
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Ownership    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_567-8_response.json  sophos-spl-local

Incorrect LiveQuery is Distributed to EDR Plugin is handled correclty
    Check EDR Plugin Installed With Base
    Simulate Live Query  FailedRequestProcesses.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  LiveQuery Log Contains  Query with name: Incorrect Query and corresponding id: 123-4 failed to execute with error: no such table: proc

EDR plugin Can Start Osquery
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Running

Test EDR Serialize Response Handles Non-UTF8 Characters in Osquery Response
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Running

    Copy File  ${TEST_INPUT_PATH}/componenttests/LiveQueryReport  ${COMPONENT_ROOT_PATH}/extensions/
    Run Process  chmod  +x  ${COMPONENT_ROOT_PATH}/extensions/LiveQueryReport
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Run Non-UTF8 Query


*** Keywords ***
Run Non-UTF8 Query
    ${result} =  Run Process  ${COMPONENT_ROOT_PATH}/extensions/LiveQueryReport  ${COMPONENT_ROOT_PATH}/var/osquery.sock  select '1\xfffd' as h;  shell=true

    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0

    Should Contain  ${result.stdout}   "errorCode": 0

EDR plugin Configures OSQuery To Enable SysLog Event Collection
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Running

    Should Exist  ${SOPHOS_INSTALL}/shared/syslog_pipe
    Check Ownership  ${SOPHOS_INSTALL}/shared/syslog_pipe  root syslog
    File Should Exist  /etc/rsyslog.d/rsyslog_sophos-spl.conf