*** Settings ***
Documentation    Suite description
Library   OperatingSystem
Library   Process

Library    ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py

Resource  McsRouterResources.robot


Test Setup  Test Setup
Test Teardown  Test Teardown

Suite Setup       Run Keywords
...               Setup MCS Tests

Suite Teardown    Run Keywords
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_TESTS DATAFEED

*** Test Cases ***
Basic XDR Datafeed Sent
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Xdr Datafeed Result  1  20200101

    log to console  sleeping
    sleep  1000


#    ${expected_body} =  Send EDR Response     LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
#    Check Cloud Server Log For EDR Response   LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
#    Check Cloud Server Log For EDR Response Body   LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1  ${expected_body}
#    Cloud Server Log Should Not Contain  Failed to decompress response body content


#MCSRouter Handles Response File With Special Characters Without Crashing
#    ${tempdir} =  Add Temporary Directory  workspace
#    Make Garbage File  ${tempdir}/garbage_file
#
#    Override LogConf File as Global Level  DEBUG
#    Register With Local Cloud Server
#    Check Correct MCS Password And ID For Local Cloud Saved
#    Start MCSRouter
#    ${mcsrouter_pid_1} =  Get MCSRouter PID
#    # ${expected_body} =  send_borked_edr_response     LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
#    Move File  ${tempdir}/garbage_file  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_f291664d-112a-328b-e3ed-f920012cdea1_response.json
#
#    Wait Until Keyword Succeeds
#    ...  10s
#    ...  2s
#    ...  Check Mcsrouter Log Contains  Failed to load response json file "/opt/sophos-spl/base/mcs/response/LiveQuery_f291664d-112a-328b-e3ed-f920012cdea1_response.json". Error: 'utf-8' codec can't decode byte
#    Require No Unhandled Exception
#    ${mcsrouter_pid_2} =  Get MCSRouter PID
#    Should Be Equal  ${mcsrouter_pid_1}  ${mcsrouter_pid_2}
#
#Test Response Given 500 From Central Is Not Retried
#    Override LogConf File as Global Level  DEBUG
#    Register With Local Cloud Server
#    Check Correct MCS Password And ID For Local Cloud Saved
#    Start MCSRouter
#    Send Command From Fake Cloud    error/server500
#    ${expected_body} =  Send EDR Response     LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
#    Check Cloud Server Log For EDR Response   LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
#    Check Cloud Server Log Contains   Internal Server Error
#    Wait Until Keyword Succeeds
#    ...  10s
#    ...  2s
#    ...  Check MCSRouter Log Contains  Discarding response 'f291664d-112a-328b-e3ed-f920012cdea1' due to rejection by central
#

*** Keywords ***
Test Teardown
    MCSRouter Default Test Teardown
    Stop Local Cloud Server
    Cleanup Temporary Folders

Test Setup
    Start Local Cloud Server

Make Garbage File
    [Arguments]  ${destination}
    ${r} =  Run Process  dd  if\=/dev/urandom  of\=${destination}  bs\=1M  count\=10
    Should Be Equal As Strings  ${r.rc}  0

Get MCSRouter PID
    ${r} =  Run Process  pgrep  -f  mcsrouter
    Should Be Equal As Strings  ${r.rc}  0
    [Return]  ${r.stdout}