*** Settings ***
Documentation    Suite description
Library   OperatingSystem
Library   Process

Resource  McsRouterResources.robot

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