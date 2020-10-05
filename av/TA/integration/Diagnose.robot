*** Settings ***
Documentation    Integration tests if SSPL-AV diagnose
Force Tags      INTEGRATION  DIAGNOSE
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ThreatReportUtils.py

Resource        ../shared/AVResources.robot
Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/DiagnoseResources.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Test Cases ***

Diagnose collects the correct files
    Check AV Plugin Installed With Base
    Configure and check scan now
    Run Diagnose
    Check Diagnose Tar Created
    Check Diagnose Collects Correct AV Files
    Check Diagnose Logs
    Remove Directory  /tmp/DiagnoseOutput  true
