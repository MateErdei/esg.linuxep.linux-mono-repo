*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem
Library         XML

Resource        AVResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      No Operation
Test Teardown   AV And Base Teardown

*** Variables ***
${SCAN_NOW_XML}       ${RESOURCES_PATH}/SAV_action_scan-now.xml

*** Test Cases ***
AV plugin Can Start sophos_threat_detector
    Check AV Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check sophos_threat_detector Running

AV plugin Can ScanNow and (fake) Report To Central
    Mock Scan Now
    Wait Until Created  ${SOPHOS_INSTALL}/base/mcs/action/scan-now.xml

    Wait Until Keyword Succeeds
    ...  45 secs
    ...  1 secs
    ...  AV Plugin Log Contains  Received new Action

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  AV Plugin Log Contains  Starting scan

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check For Scan Complete XML

    Verify Event XML

*** Keywords ***
Mock Scan Complete
    copy file  ${SOPHOS_INSTALL}/base/mcs/event/*.xml

Mock Scan Now
    copy file  ${SCAN_NOW_XML}  ${SOPHOS_INSTALL}/base/mcs/action/

Check Scan Now Has Started

Check For Scan Complete
    Should Exist  ${SOPHOS_INSTALL}/base/mcs/event/*.xml
    List Files In Directory  ${SOPHOS_INSTALL}/base/mcs/event/

Verify Event XML
    ${SCAN_COMPLETE_XML}  parse xml  ${SOPHOS_INSTALL}/base/mcs/event/*.xml
    ELEMENT TEXT SHOULD BE  source=${root}  expected=<scanComplete>  xpath=scanComplete

Configure Scan Exclusions Everything Else # Will allow for one directory to be selected during a scan
