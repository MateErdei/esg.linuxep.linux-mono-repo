*** Settings ***
Documentation    Suite description

Force Tags  FUZZ

Library     OperatingSystem

Library     ${COMMON_TEST_LIBS}/FuzzerSupport.py

Resource    ${COMMON_TEST_ROBOT}/FuzzTestsResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot

Suite Setup   Fuzzer Tests Global Setup
Suite Teardown   Fuzzer Tests Global TearDown

Test Setup   Run Keywords
...          Fuzzer Set Paths  ${EVEREST-BASE}  ${SUPPORT_FILES}/base_data/fuzz    AND
...          Ensure Alf Fuzzer Targets Built

Test Teardown  Run Keywords
...            Log File   /tmp/fuzz.log   AND
...            Remove file  /tmp/fuzz.log  AND
...            General Test Teardown

*** Test Cases ***

Test Parse ALC Policy
    ${result}=  Run ALF Fuzzer By Name  parsealcpolicytests
    Should Be Equal As Strings   ${result}  0

Test Suldownloader Config
    ${result}=  Run ALF Fuzzer By Name  suldownloaderconfigtests
    Should Be Equal As Strings   ${result}  0

Test DownloadReport
    ${result}=  Run ALF Fuzzer By Name  downloadreporttests
    Should Be Equal As Strings   ${result}  0
