*** Settings ***
Documentation    Suite description

Default Tags  FUZZ

Library           OperatingSystem
Library    ${LIBS_DIRECTORY}/FuzzerSupport.py
Resource  ../GeneralTeardownResource.robot
Resource  FuzzTestsResources.robot


Test Setup   Ensure Alf Fuzzer Targets Built

Test Teardown  Run Keywords
...            Log File   /tmp/fuzz.log   AND
...            Remove file  /tmp/fuzz.log  AND
...            General Test Teardown

*** Test Cases ***

Test Process LiveQuery Policy
    ${result}=  Run ALF Fuzzer By Name  processlivequerypolicytests
    Should Be Equal As Strings   ${result}  0

*** Keywords ***
Fuzzer Tests Global Setup
        Fuzzer Set Paths  tests/FuzzerTests/AflFuzzScripts/data/
        Setup Base Build

Fuzzer Tests Global TearDown
     Remove file  /tmp/fuzz.log