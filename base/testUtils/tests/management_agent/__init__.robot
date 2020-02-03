*** Settings ***
Suite Setup      Require Fresh Install
Suite Teardown   Uninstall SSPL

Test Setup        Management Agent Test Setup
Test Teardown     Run Keywords
...               Terminate All Processes  kill=True  AND
...               General Test Teardown

Library    Process
Library    OperatingSystem

Resource    ManagementAgentResources.robot
Resource    ../GeneralTeardownResource.robot
