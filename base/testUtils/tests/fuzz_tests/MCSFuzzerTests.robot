*** Settings ***
Documentation    Fuzzer tests for MCS

Default Tags  MCS_FUZZ

Library    Process
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FuzzerSupport.py


Resource   ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource   ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup     Run Keywords
...             Regenerate Certificates

Suite Teardown  Run Keywords
...             Cleanup Certificates

Test Setup      Setup MCS Tests
Test Teardown   Run Keywords
...             Kill MCS Fuzzer  AND
...             Stop Local Cloud Server  AND
...             MCSRouter Default Test Teardown  AND
...             Dump Kittylogs Dir Contents  AND
...             Stop System Watchdog

Test Timeout  150 minutes


*** Variables ***
${MCS_FUZZER_PATH}   ${SUPPORT_FILES}/fuzz_tests/mcs_fuzz_test_runner.py

*** Test Cases ***

Test MCS Policy Fuzzer
    Run MCS Router Fuzzer   mcs  8

Test ALC Policy Fuzzer
    Run MCS Router Fuzzer  alc  9

*** Keywords ***
Run MCS Router Fuzzer
    [Arguments]   ${Suite}  ${MutationsToTestRatio}
    Start Mcs Fuzzer   ${MCS_FUZZER_PATH}   ${Suite}  ${MutationsToTestRatio}
    Start Fuzzed Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/Cloud_MCS_policy_ShortCmdPollingDelay.xml
    #wait just less than the default timeout
    Wait For Mcs Fuzzer   7200