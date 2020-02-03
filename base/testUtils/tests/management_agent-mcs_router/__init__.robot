*** Settings ***

Resource  ../installer/InstallerResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Library    ${libs_directory}/MCSRouter.py

Suite Setup      Setup MCS And Management Agent Tests
Suite Teardown   Cleanup MCS And Management Agent Tests

Test Setup        Set Test Variable    ${tmpdir}     ./tmp
Test Teardown     MCSRouter Default Test Teardown

*** Keywords ***

Setup MCS And Management Agent Tests
    Regenerate Certificates
    Setup MCS Tests
    Start Local Cloud Server

Cleanup MCS And Management Agent Tests
    Stop Local Cloud Server
    Run Process    make   cleanCerts    cwd=./SupportFiles/CloudAutomation/
    Uninstall SSPL

