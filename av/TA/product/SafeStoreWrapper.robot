*** Settings ***
Documentation   Tests for SafeStore wrapper, runs a gtest binary
Force Tags      PRODUCT  SAFESTORE  TAP_PARALLEL1

Resource    ../shared/AVResources.robot

Library         OperatingSystem
Library         Process
Library         ../Libs/OnFail.py

Test Setup      No Operation
Test Teardown   OnFail.run_teardown_functions

*** Test Cases ***
SafeStoreWrapper Tests
    register on fail  List Files in AV Test Tools
    ${safestore_tests_binary} =   Set Variable  ${AV_TEST_TOOLS}/SafeStoreTapTests
    ${result} =   Run Process   ${safestore_tests_binary}
    ...  env:LD_LIBRARY_PATH=/opt/sophos-spl/plugins/av/lib64:${COMPONENT_SDDS_COMPONENT}/files/plugins/av/lib64
    ...  stderr=STDOUT
    Log  ${result.stdout}
    Should Be Equal As Integers   ${result.rc}  ${0}

*** Keywords ***
List Files in AV Test Tools
    ${Files} =  List Files In Directory  ${AV_TEST_TOOLS}
    log  ${Files}
