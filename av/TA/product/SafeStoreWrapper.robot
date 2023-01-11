*** Settings ***
Documentation   Tests for SafeStore wrapper, runs a gtest binary
Force Tags      PRODUCT  SAFESTORE

Resource    ../shared/AVResources.robot

Library         OperatingSystem
Library         Process

Test Setup      No Operation

*** Test Cases ***
SafeStoreWrapper Tests
    ${Files} =  List Files In Directory  ${AV_TEST_TOOLS}
    log  ${Files}
    ${safestore_tests_binary} =   Set Variable  ${AV_TEST_TOOLS}/SafeStoreTapTests
    ${result} =   Run Process   ${safestore_tests_binary}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}
