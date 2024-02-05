*** Settings ***
Documentation    Deliberately generate a core file!

Force Tags       MANUAL  TAP_PARALLEL2

Library  Process
Library  ${COMMON_TEST_LIBS}/OnFail.py
Library  ${COMMON_TEST_LIBS}/CoreDumps.py

Resource    ../shared/AVResources.robot
Resource    ../shared/DumpLog.robot

Test Teardown   OnFail.Run Cleanup Functions

*** Test Cases ***

Deliberately generate a core file
    Run Process  chmod  755  ${RESOURCES_PATH}/TinyCoreFile
    Run Process  ${RESOURCES_PATH}/TinyCoreFile
