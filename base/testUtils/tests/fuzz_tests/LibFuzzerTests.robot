*** Settings ***
Documentation    Fuzz Tests

Default Tags  FUZZ

Library    ../libs/FuzzerSupport.py
Resource  ../GeneralTeardownResource.robot
Resource  FuzzTestsResources.robot

Test Setup   Run Keywords
...          Fuzzer Set Paths  ${EVEREST-BASE}  ${SUPPORT_FILES}/base_data/fuzz  AND
...          Ensure Fuzzer Targets Built

Suite Setup   Fuzzer Tests Global Setup
Suite Teardown   Fuzzer Tests Global TearDown

*** Test Cases ***
Test ZMQ Stack
    Run Fuzzer By Name  ZMQTests

Test Plugin API
    [Tags]  FUZZ  TESTFAILURE
    #TODO  LINUXDAR-888
    Run Fuzzer By Name  PluginApiTest

Test Management Agent API
    Run Fuzzer By Name  ManagementAgentApiTest

Test Fuzz Simple Functions
    Run Fuzzer By Name  SimpleFunctionTests

Test Fuzz Watchdog API
    Run Fuzzer By Name  WatchdogApiTest


