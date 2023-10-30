*** Settings ***
Suite Setup      Require Fresh Install
Suite Teardown   Uninstall SSPL

Test Setup        Management Agent Test Setup
Test Teardown     Run Keywords
...               Terminate All Processes  kill=True  AND
...               General Test Teardown

Library    Process
Library    OperatingSystem

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot
