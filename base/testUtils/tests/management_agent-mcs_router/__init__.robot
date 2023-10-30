*** Settings ***

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Library    ${COMMON_TEST_LIBS}/MCSRouter.py

Suite Setup      Setup MCS And Management Agent Tests
Suite Teardown   Cleanup MCS And Management Agent Tests

Test Setup        Set Test Variable    ${tmpdir}     ./tmp
Test Teardown    Test Fake Plugin Teardown

*** Keywords ***
Setup MCS And Management Agent Tests
    Regenerate Certificates
    Setup MCS Tests
    Start Local Cloud Server

Cleanup MCS And Management Agent Tests
    Stop Local Cloud Server
    Run Process    make   cleanCerts    cwd=${SUPPORT_FILES}/CloudAutomation/
    Uninstall SSPL

