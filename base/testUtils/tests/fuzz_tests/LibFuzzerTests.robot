*** Settings ***
Documentation    Fuzz Tests

Force Tags  FUZZ

Library    ${COMMON_TEST_LIBS}/FuzzerSupport.py

Resource  ${COMMON_TEST_ROBOT}/FuzzTestsResources.robot
Resource  ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot

Test Setup   Run Keywords
...          Fuzzer Set Paths  ${EVEREST-BASE}  ${SUPPORT_FILES}/base_data/fuzz  AND
...          Ensure Fuzzer Targets Built

Suite Setup   Fuzzer Tests Global Setup
Suite Teardown   Fuzzer Tests Global TearDown

*** Test Cases ***
# TODO: LINUXDAR-4905: Fix and untag TESTFAILURE for this test
Test ZMQ Stack
    [Tags]  TESTFAILURE
    ${failed}=  Run Fuzzer By Name  ZMQTests
    Should Not Be True    ${failed}

Test Plugin API
    ${failed}=  Run Fuzzer By Name  PluginApiTest
    Should Not Be True    ${failed}

Test Management Agent API
    ${failed}=  Run Fuzzer By Name  ManagementAgentApiTest
    Should Not Be True    ${failed}

Test Fuzz Simple Functions
    ${failed}=  Run Fuzzer By Name  SimpleFunctionTests
    Should Not Be True    ${failed}

Test Action Runner
    ${failed}=  Run Fuzzer By Name  ActionRunner
    Should Not Be True    ${failed}

# TODO: LINUXDAR-4925: Fix and untag TESTFAILURE for this test
Test Fuzz Watchdog API
    [Tags]  TESTFAILURE
    ${failed}=  Run Fuzzer By Name  WatchdogApiTest
    Should Not Be True    ${failed}
