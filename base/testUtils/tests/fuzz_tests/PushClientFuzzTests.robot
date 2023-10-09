*** Settings ***
Documentation    Fuzzer tests for MCS

Default Tags  MCS_FUZZ

Library    Process
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FuzzerSupport.py


Resource   ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot

Suite Setup     Run Keywords
...             Regenerate Certificates  AND
...             Set Timeout

Suite Teardown  Run Keywords
...             Cleanup Certificates

Test Setup      Setup MCS Tests
Test Teardown   Run Keywords
...             Kill Push Fuzzer  AND
...             Stop Local Cloud Server  AND
...             Check Mcsrouter Log Does Not Contain   Failed to write an action to:   AND
...             MCSRouter Default Test Teardown  AND
...             Dump Kittylogs Dir Contents  AND
...             Dump Push Server Log   AND
...             Stop System Watchdog

Test Timeout  100 minutes

*** Variables ***
${MCS_FUZZER_PATH}   ${SUPPORT_FILES}/push_fuzzer/runner.py

*** Test Cases ***
Test Push Livequery Command Fuzz
    Run MCS Router Fuzzer   livequery

Test Push Wakeup Command Fuzz
    Run MCS Router Fuzzer   wakeup

Test Push Livequery Response Fuzz
    #Force logging of response sent to central in cloudServer.log
    Override LogConf File as Global Level  DEBUG
    Run MCS Router Fuzzer   response  10
    # check no response file left in response directory
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/response

*** Keywords ***
Run MCS Router Fuzzer
    [Arguments]   ${Suite}   ${MutationsToTestRatio}=1

    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Should Exist  ${REGISTER_CENTRAL}
    Register And Wait For Registration With Local Cloud Server

    Start Mcs Fuzzer   ${MCS_FUZZER_PATH}   ${Suite}  ${MutationsToTestRatio}
    Wait For Mcs Fuzzer   ${MCS_FUZZER_TIMEOUT}

Register And Wait For Registration With Local Cloud Server
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Check Register Central Log Contains  Now managed by Sophos Central
    Check Correct MCS Password And ID For Local Cloud Saved

Kill Push Fuzzer
    Run Process  pgrep runner.py | xargs kill -9  shell=true

Set Timeout
    ${placeholder} =  Get Environment Variable   MCSFUZZ_TIMEOUT  default=3000
    Set Suite Variable  ${MCS_FUZZER_TIMEOUT}  ${placeholder}