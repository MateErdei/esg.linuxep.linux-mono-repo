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

*** Test Cases ***
AV plugin Can Start sophos_threat_detector
    Check AV Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check sophos_threat_detector Running

AV plugin runs scan now
    Check AV Plugin Installed With Base
    Send Sav Policy To Base  SAV_Policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains  Completed scan scanNow
    AV Plugin Log Contains  Starting Scan Now scan
    AV Plugin Log Contains  Starting scan scanNow

AV plugin fails scan now if no policy
    Check AV Plugin Installed With Base
    Send Sav Action To Base  ScanNow_Action.xml
    AV Plugin Log Does Not Contain  Starting scan scanNow
    AV Plugin Log Contains  Starting Scan Now scan

AV plugin sends Scan Complete event and (fake) Report To Central
    Check AV Plugin Installed With Base
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until Management Log Contains  Action /opt/sophos-spl/base/mcs/action/SAV_action
    Wait Until AV Plugin Log Contains  Starting scan
    # Check For Scan Complete
    # This will parse the scan complete xml in the events folder and compare it to the one we have in our resouces folder
    # Once we can validate xml appearing in the events folder, we can expect this to be sent to Central (In a System Product Test)