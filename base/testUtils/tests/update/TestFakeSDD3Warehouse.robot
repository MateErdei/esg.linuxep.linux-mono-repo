*** Settings ***
Library     ${COMMON_TEST_LIBS}/CentralUtils.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py

Resource    ${COMMON_TEST_ROBOT}/SDDS3Resources.robot
Resource    ${COMMON_TEST_ROBOT}/ThinInstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Test Setup   Generate Fake sdds3 warehouse
Test Teardown  sdds3 Teardown

Force Tags    MANUAL
Test Timeout    NONE

*** Keywords ***
sdds3 Teardown
    Clean up fake warehouse
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed   Dump Cloud Server Log
    Stop Local Cloud Server
    Cleanup Local Cloud Server Logs
    Teardown Reset Original Path
    Run Keyword If Test Failed    Dump Thininstaller Log
    Remove Thininstaller Log
    Stop Local SDDS3 Server
*** Test Cases ***
Run Sdds warehouse
    ${Files} =  List Files In Directory  ${SDDS3_FAKEFLAGS}
    Log  ${Files}
    ${Files} =  List Files In Directory  ${SDDS3_FAKEPACKAGES}
    Log  ${Files}
    ${Files} =  List Files In Directory  ${SDDS3_FAKESUITES}
    Log  ${Files}
    Setup_MCS_Cert_Override
    Start Local Cloud Server
    Get Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Run Default Thininstaller  0