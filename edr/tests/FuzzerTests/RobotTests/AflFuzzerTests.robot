*** Settings ***
Documentation    Suite description

Default Tags  FUZZ

Library           OperatingSystem
Library    FuzzerSupport.py


Test Setup   Run Keywords
...          Fuzzer Set Paths  tests/FuzzerTests/AflFuzzScripts/data/    AND
...          Ensure Alf Fuzzer Targets Built

Test Teardown  Run Keywords
...            Log File   /tmp/fuzz.log   AND
...            Remove file  /tmp/fuzz.log


*** Test Cases ***

Test Process LiveQuery Policy
    ${result}=  Run ALF Fuzzer By Name  processlivequerypolicytests
    Should Be Equal As Strings   ${result}  0

