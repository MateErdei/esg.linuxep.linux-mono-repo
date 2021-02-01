*** Settings ***
Documentation    Suite description

Default Tags  FUZZ

Library           OperatingSystem
Library    FuzzerSupport.py


Test Setup   Ensure Alf Fuzzer Targets Built

Test Teardown  Run Keywords
...            Log File   /tmp/fuzz.log   AND
...            Remove file  /tmp/fuzz.log

Suite Setup   Fuzzer Tests Global Setup
Suite Teardown   Fuzzer Tests Global TearDown
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