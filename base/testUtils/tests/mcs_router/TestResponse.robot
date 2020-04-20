*** Settings ***
Documentation    Suite description
Library   OperatingSystem
Library   Process

Resource  McsRouterResources.robot

Test Setup  Test Setup
Test Teardown  Test Teardown

Suite Setup       Run Keywords
...               Setup MCS Tests

Suite Teardown    Run Keywords
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

test 500
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Command From Fake Cloud    error/server500
    ${expected_body} =  Send EDR Response     LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
    Check Cloud Server Log For EDR Response   LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
    Check Cloud Server Log Contains   Internal Server Error
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check MCSRouter Log Contains  Discarding response 'f291664d-112a-328b-e3ed-f920012cdea1' due to rejection by central


*** Keywords ***
Test Teardown
    MCSRouter Default Test Teardown
    Stop Local Cloud Server

Test Setup
    Start Local Cloud Server