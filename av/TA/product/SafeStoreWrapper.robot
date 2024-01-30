*** Settings ***
Documentation   Tests for SafeStore wrapper, runs a gtest binary
Force Tags      PRODUCT  SAFESTORE  TAP_PARALLEL1

Resource    ../shared/AVResources.robot

Library         OperatingSystem
Library         Process
Library         ${COMMON_TEST_LIBS}/OnFail.py
Library         ../Libs/TapTestOutput.py

Test Setup      No Operation
Test Teardown   OnFail.run_teardown_functions

*** Test Cases ***
SafeStoreWrapper Tests
    ${AV_TEST_TOOLS} =  TapTestOutput.Extract Tap Test Output
    register on fail  List Files in AV Test Tools  ${AV_TEST_TOOLS}
    ${safestore_tests_binary} =   Set Variable  ${AV_TEST_TOOLS}/SafeStoreTapTests
    ${result} =   Run Process   ${safestore_tests_binary}
    ...  env:LD_LIBRARY_PATH=/opt/sophos-spl/plugins/av/lib64:${COMPONENT_SDDS_COMPONENT}/files/plugins/av/lib64
    ...  stderr=STDOUT
    Log  ${result.stdout}
    Should Be Equal As Integers   ${result.rc}  ${0}

*** Keywords ***
List Files in AV Test Tools
    [Arguments]  ${AV_TEST_TOOLS}
    ${Files} =  List Files In Directory  ${AV_TEST_TOOLS}
    log  ${Files}
