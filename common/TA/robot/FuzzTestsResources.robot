*** Settings ***
Documentation    Resource file for fuzz tests

Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/FuzzerSupport.py
Library    ${COMMON_TEST_LIBS}/PathManager.py


*** Keywords ***
Fuzzer Tests Global Setup
        ${placeholder}=  Get Repo Root Path
        Set Global Variable  ${EVEREST-BASE}  ${placeholder}
        Directory Should Exist  ${EVEREST-BASE}
        Fuzzer Set Paths  ${EVEREST-BASE}  ${SUPPORT_FILES}/base_data/fuzz
        Setup Base Build

Fuzzer Tests Global TearDown
     Remove file  /tmp/fuzz.log