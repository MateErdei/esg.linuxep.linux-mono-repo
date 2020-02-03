*** Settings ***
Documentation    Suite description

Default Tags  FUZZ

Library           OperatingSystem
Library    ../libs/FuzzerSupport.py
Resource  ../GeneralTeardownResource.robot
Resource  FuzzTestsResources.robot

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

Test Suldownloader Report
    ${result}=  Run ALF Fuzzer By Name  suldownloaderreporttests
    Should Be Equal As Strings   ${result}  0
