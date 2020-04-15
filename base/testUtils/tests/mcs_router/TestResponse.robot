*** Settings ***
Documentation    Suite description
Library   OperatingSystem
Library   Process

Library    ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py

Resource  McsRouterResources.robot

Test Teardown  Test Teardown

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server

Suite Teardown    Run Keywords
...               Stop Local Cloud Server  AND
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_TESTS

*** Test Cases ***
Basic EDR Response Sent
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    ${expected_body} =  Send EDR Response     LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
    Check Cloud Server Log For EDR Response   LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
    Check Cloud Server Log For EDR Response Body   LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1  ${expected_body}
    Cloud Server Log Should Not Contain  Failed to decompress response body content


test1
    ${tempdir} =  Add Temporary Directory  workspace
    Make Garbage File  ${tempdir}/garbage_file

    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    # ${expected_body} =  send_borked_edr_response     LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
    Move File  ${tempdir}/garbage_file  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_f291664d-112a-328b-e3ed-f920012cdea1_response.json

    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Mcsrouter Log Contains  Failed to read response json file "/opt/sophos-spl/base/mcs/response/LiveQuery_f291664d-112a-328b-e3ed-f920012cdea1_response.json". Error: 'utf-8' codec can't decode byte
    Require No Unhandled Exception

*** Keywords ***
Make Garbage File
    [Arguments]  ${destination}
    ${r} =  Run Process  dd  if\=/dev/urandom  of\=${destination}  bs\=1M  count\=10
    Should Be Equal As Strings  ${r.rc}  0

Test Teardown
    MCSRouter Default Test Teardown
    Cleanup Temporary Folders