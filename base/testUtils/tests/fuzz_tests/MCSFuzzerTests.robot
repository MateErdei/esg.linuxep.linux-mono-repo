*** Settings ***
Documentation    Fuzzer tests for MCS

Default Tags  MCS_FUZZ

Library    Process
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FuzzerSupport.py


Resource   ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot
Resource  ../mdr_plugin/MDRResources.robot

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
...             Stop System Watchdog  AND

Test Timeout  210 minutes


*** Variables ***
${MCS_FUZZER_PATH}   ${SUPPORT_FILES}/fuzz_tests/mcs_fuzz_test_runner.py

*** Test Cases ***

Test MCS Policy Fuzzer
    Run MCS Router Fuzzer   mcs  4

Test MDR Policy Fuzzer
    Start Watchdog
    Install MTR From Fake Component Suite
    Stop System Watchdog
    Run MCS Router Fuzzer   mdr  2

Test ALC Policy Fuzzer
    Run MCS Router Fuzzer  alc  6

*** Keywords ***
Run MCS Router Fuzzer
    [Arguments]   ${Suite}  ${MutationsToTestRatio}
    Start Mcs Fuzzer   ${MCS_FUZZER_PATH}   ${Suite}  ${MutationsToTestRatio}
    Start Fuzzed Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/Cloud_MCS_policy_ShortCmdPollingDelay.xml
    Wait For Mcs Fuzzer