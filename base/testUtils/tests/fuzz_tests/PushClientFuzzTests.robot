*** Settings ***
Documentation    Fuzzer tests for MCS

Default Tags  MCS_FUZZ

Library    Process
#Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FuzzerSupport.py


Resource   ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot

Suite Setup     Run Keywords
...             Regenerate Certificates

Suite Teardown  Run Keywords
...             Cleanup Certificates

Test Setup      Setup MCS Tests
Test Teardown   Run Keywords
...             Kill Push Fuzzer  AND
...             Stop Local Cloud Server  AND
...             MCSRouter Default Test Teardown  AND
...             Dump Kittylogs Dir Contents  AND
...             Stop System Watchdog

Test Timeout  5 minutes

*** Variables ***
${MCS_FUZZER_PATH}   ${SUPPORT_FILES}/push_fuzzer/runner.py

*** Test Cases ***

Test Push Fuzzer
    Run MCS Router Fuzzer


*** Keywords ***
Run MCS Router Fuzzer
    ${handle} =  Start Process   /usr/bin/python3   ${MCS_FUZZER_PATH}   alias=push_fuzzer
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Should Exist  ${REGISTER_CENTRAL}
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    #wait just less than the default timeout
    ${result} =    Wait For Process    push_fuzzer  timeout=30 s  on_timeout=terminate

Kill Push Fuzzer
    Run Process  pgrep runner.py | xargs kill -9  shell=true