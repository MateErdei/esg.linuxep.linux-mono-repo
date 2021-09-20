*** Settings ***
Documentation    Fuzz Tests

Default Tags  FUZZ

Library    FuzzerSupport.py
Library    OperatingSystem

Test Setup   Run Keywords
...          Fuzzer Set Paths  tests/FuzzerTests/data/   AND
...          Ensure Fuzzer Targets Built

*** Test Cases ***
Test LiveQuery
    Run Fuzzer By Name  LiveQueryTests

Test LiveQueryInput
    Run Fuzzer By Name  LiveQueryInputTests
