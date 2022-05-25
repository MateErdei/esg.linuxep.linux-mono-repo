*** Settings ***
Documentation    Fuzz Tests

Default Tags  FUZZ

Library    ${LIBS_DIRECTORY}/FuzzerSupport.py
Resource  ../GeneralTeardownResource.robot
Resource  FuzzTestsResources.robot

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

Test Fuzz Watchdog API
    ${failed}=  Run Fuzzer By Name  WatchdogApiTest
    Should Not Be True    ${failed}
