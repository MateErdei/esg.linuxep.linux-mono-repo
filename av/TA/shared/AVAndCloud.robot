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
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log


*** Test Cases ***
AV plugin Can ScanNow and Report To Central
    Mock Scan Now
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  AV Plugin Log Contains  Received new Action

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Scan Now Has Started

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check For Scan Complete XML

    Verify Event XML

*** Keywords ***
Mock Scan Complete
    copy file  ${SOPHOS_INSTALL}/base/mcs/event/*.xml

Mock Scan Now
    copy file  ${SOPHOS_INSTALL}/base/mcs/policy/*.xml

Check Scan Now Has Started

Check For Scan Complete
    ## Find event XML file
    ## Verify event XML is valid
    ${eventXmlRoot} = parse xml  ${SCAN_NOW_XML}  keep_clark_notation=True
    ELEMENT TEXT SHOULD BE  source=${root}  expected=<scanComplete>  xpath=scanComplete

Verify Event XML

Configure Scan Exclusions Everything Else ## Will allow for one directory to be selected during a scan










