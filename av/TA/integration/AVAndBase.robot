*** Settings ***
Documentation    Suite description

Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py

Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot

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
    Send Sav Policy To Base  SAV_Policy.xml
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until Management Log Contains  Action /opt/sophos-spl/base/mcs/action/SAV_action
    Wait Until AV Plugin Log Contains  Completed scan
    Wait Until AV Plugin Log Contains  Starting scan
    Wait Until AV Plugin Log Contains  Sending scan complete
    Validate latest Event

Diagnose collects the correct files
    Check AV Plugin Installed With Base
    Run Diagnose
    Check Diagnose Tar Created
    Check Diagnose Collects Correct AV Files
    Check Diagnose Logs

AV Plugin uninstalls
    Check avscanner in /usr/local/bin
    Run uninstaller
    Check avscanner not in /usr/local/bin


*** Keywords ***

Check avscanner in /usr/local/bin
    File Should Exist  /usr/local/bin/avscanner

Check avscanner not in /usr/local/bin
    File Should Not Exist  /usr/local/bin/avscanner

Run uninstaller
    Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh

Validate latest Event
     ${eventXml}=  get_latest_xml_from_events  base/mcs/event/
     ${parsedXml}=  parse xml  ${eventXml}
     ELEMENT TEXT SHOULD MATCH  source=${parsedXml}  pattern=scanNow  normalize_whitespace=True  xpath=scanComplete

