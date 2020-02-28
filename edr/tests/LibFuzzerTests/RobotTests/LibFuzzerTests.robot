*** Settings ***
Documentation    Fuzz Tests

Default Tags  FUZZ

Library    FuzzerSupport.py
Library    OperatingSystem

Test Setup   Run Keywords
...          Fuzzer Set Paths  tests/LibFuzzerTests/data/   AND
...          Ensure Fuzzer Targets Built

Suite Setup   Fuzzer Tests Global Setup
Suite Teardown   Fuzzer Tests Global TearDown

*** Test Cases ***
Test LiveQuery
    Run Fuzzer By Name  LiveQueryTests

*** Keywords ***
Fuzzer Tests Global Setup
        Fuzzer Set Paths  tests/LibFuzzerTests/data/
        Setup Base Build

Fuzzer Tests Global TearDown
     Remove file  /tmp/fuzz.log
