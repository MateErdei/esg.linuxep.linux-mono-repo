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

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      Diagnose Test Setup
Test Teardown   Diagnose Test TearDown

*** Keywords ***

Diagnose Test Setup
    AV And Base Setup

Diagnose Test TearDown
    Run Teardown Functions
    AV And Base Teardown

*** Test Cases ***

Diagnose collects the correct files
    Configure and check scan now with offset
    Run Diagnose
    Check Diagnose Tar Created
    Check Diagnose Collects Correct AV Files
    Check Diagnose Logs
