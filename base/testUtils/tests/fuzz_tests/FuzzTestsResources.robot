*** Settings ***
Documentation    Resource file for fuzz tests

Library    ../libs/FuzzerSupport.py
Library           OperatingSystem


*** Keywords ***
Fuzzer Tests Global Setup
        ${placeholder}=  Normalize Path  ${libs_directory}/../../everest-base
        Set Global Variable  ${EVEREST-BASE}  ${placeholder}
        Directory Should Exist  ${EVEREST-BASE}
        Fuzzer Set Paths  ${EVEREST-BASE}  ${SUPPORT_FILES}/base_data/fuzz
        Setup Base Build

Fuzzer Tests Global TearDown
     Remove file  /tmp/fuzz.log