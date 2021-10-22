*** Settings ***
Library  FuzzerSupport.py
Library  OperatingSystem

Suite Setup     Fuzzer Tests Global Setup
Suite Teardown  Fuzzer Tests Global TearDown

*** Keywords ***
Fuzzer Tests Global Setup
    Fuzzer Set Paths  tests/FuzzerTests/AflFuzzScripts/data/
    Setup Base Build

Fuzzer Tests Global TearDown
    Remove file  /tmp/fuzz.log
