*** Settings ***
Documentation    Integration tests if SSPL-AV diagnose
Force Tags      INTEGRATION  DIAGNOSE
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/ThreatReportUtils.py

Resource        ../shared/AVResources.robot
Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/DiagnoseResources.robot
Resource        ../shared/ErrorMarkers.robot

Suite Setup     Install With Base SDDS
Suite Teardown   Uninstall All

Test Setup      Diagnose Test Setup
Test Teardown   Diagnose Test TearDown

*** Keywords ***

Diagnose Test Setup
    AV And Base Setup

Diagnose Test TearDown
    Exclude CustomerID Failed To Read Error
    Exclude MCS Router is dead
    Check All Product Logs Do Not Contain Error
    Run Teardown Functions
    AV And Base Teardown

*** Test Cases ***

Diagnose collects the correct files
    Configure and check scan now with offset
    Run Diagnose
    Check Diagnose Tar Created
    Check Diagnose Collects Correct AV Files
    Check Diagnose Logs
