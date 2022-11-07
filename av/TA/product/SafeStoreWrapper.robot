*** Settings ***
Documentation   Tests for SafeStore wrapper, runs a gtest binary
Force Tags      PRODUCT  SAFESTORE

Library         OperatingSystem
Library         Process

Test Setup     SafeStoreWrapper Test Setup
Test Teardown  SafeStoreWrapper Test Teardown

*** Keywords ***

SafeStoreWrapper Test Setup
    Set Suite Variable  ${safestore_unpacked}  /tmp/safestorewrapper
    Create Directory  ${safestore_unpacked}
    ${result} =   Run Process   tar    xzf    ${BUILD_ARTEFACTS_FOR_TAP}/tap_test_output.tar.gz    -C    ${safestore_unpacked}/
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}

SafeStoreWrapper Test Teardown
    Remove Directory    ${safestore_unpacked}   recursive=True

*** Test Cases ***

SafeStoreWrapper Tests
    ${Files} =  List Files In Directory  ${safestore_unpacked}
    log  ${Files}
    ${safestore_tests_binary} =   Set Variable  ${safestore_unpacked}/tap_test_output/SafeStoreTapTests
    ${result} =   Run Process   ${safestore_tests_binary}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}
