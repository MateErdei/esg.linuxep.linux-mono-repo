*** Settings ***
Documentation    Suite description

Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ThreatReportUtils.py

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
    Configure and check scan now

AV plugin runs scan now twice
    Check AV Plugin Installed With Base
    Configure and check scan now
    Mark AV Log
    Check scan now

AV plugin runs scan now and completes (STRESS TEST)
     Create EICAR files  1000  /tmp/stress
     Check AV Plugin Installed With Base
     Configure and check scan now
     ${result} =  Count AV Log Lines
     Should be true  ${result} > 1000
     [Teardown]  Remove Directory  /tmp/stress  True


AV plugin fails scan now if no policy
    Check AV Plugin Installed With Base
    Send Sav Action To Base  ScanNow_Action.xml
    AV Plugin Log Does Not Contain  Starting scan scanNow
    AV Plugin Log Contains  Starting Scan Now scan

AV plugin SAV Status contains revision ID of policy
    Check AV Plugin Installed With Base
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini
    Send Sav Policy To Base  SAV_Policy.xml
    Wait Until SAV Status XML Contains  Res="Same"  timeout=60
    SAV Status XML Contains  RevID="ac9eaa2f09914ce947cfb14f1326b802ef0b9a86eca7f6c77557564e36dbff9a"
    SAV Status XML Contains  <product-version>${version}</product-version>

AV plugin sends Scan Complete event and (fake) Report To Central
    Check AV Plugin Installed With Base
    Send Sav Policy To Base  SAV_Policy.xml
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until Management Log Contains  Action /opt/sophos-spl/base/mcs/action/SAV_action
    Wait Until AV Plugin Log Contains  Starting scan
    Wait Until AV Plugin Log Contains  Completed scan
    Wait Until AV Plugin Log Contains  Sending scan complete
    Validate latest Event

AV Configures No Scheduled Scan Correctly
    Check AV Plugin Installed With Base
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    Wait Until AV Plugin Log Contains  No of Scheduled Scans Configured: 0

AV Configures Single Scheduled Scan Correctly
    Check AV Plugin Installed With Base
    Send Complete Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    Wait Until AV Plugin Log Contains  No of Scheduled Scans Configured: 1
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 11:00:00
    Wait Until AV Plugin Log Contains  No of Exclusions Configured: 27
    Wait Until AV Plugin Log Contains  No of Sophos Defined Extension Exclusions Configured: 3
    Wait Until AV Plugin Log Contains  No of User Defined Extension Exclusions Configured: 4

AV Configures Multiple Scheduled Scans Correctly
    Check AV Plugin Installed With Base
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    Wait Until AV Plugin Log Contains  No of Scheduled Scans Configured: 2
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan One
    Wait Until AV Plugin Log Contains  Days: Tuesday Saturday
    Wait Until AV Plugin Log Contains  Times: 04:00:00 16:00:00
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan Two
    Wait Until AV Plugin Log Contains  Days: Monday Thursday
    Wait Until AV Plugin Log Contains  Times: 11:00:00 23:00:00
    Wait Until AV Plugin Log Contains  No of Exclusions Configured: 24
    Wait Until AV Plugin Log Contains  No of Sophos Defined Extension Exclusions Configured: 0
    Wait Until AV Plugin Log Contains  No of User Defined Extension Exclusions Configured: 0

AV Handles Scheduled Scan With Badly Configured Day
    Check AV Plugin Installed With Base
    Send Sav Policy With Invalid Scan Day
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    Wait Until AV Plugin Log Contains  Invalid day from policy: blernsday
    Wait Until AV Plugin Log Contains  No of Scheduled Scans Configured: 1
    Wait Until AV Plugin Log Contains  Days: INVALID
    Wait Until AV Plugin Log Contains  Times: 11:00:00

AV Handles Scheduled Scan With Badly Configured Time
    Check AV Plugin Installed With Base
    Send Sav Policy With Invalid Scan Time
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    AV Plugin Log Contains  No of Scheduled Scans Configured: 1
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 00:00:00

AV Reconfigures Scans Correctly
    Check AV Plugin Installed With Base
    Send Complete Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    AV Plugin Log Contains  No of Scheduled Scans Configured: 1
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 11:00:00
    Wait Until AV Plugin Log Contains  No of Exclusions Configured: 27
    Wait Until AV Plugin Log Contains  No of Sophos Defined Extension Exclusions Configured: 3
    Wait Until AV Plugin Log Contains  No of User Defined Extension Exclusions Configured: 4
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    Wait Until AV Plugin Log Contains  No of Scheduled Scans Configured: 2
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan One
    Wait Until AV Plugin Log Contains  Days: Tuesday Saturday
    Wait Until AV Plugin Log Contains  Times: 04:00:00 16:00:00
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan Two
    Wait Until AV Plugin Log Contains  Days: Monday Thursday
    Wait Until AV Plugin Log Contains  Times: 11:00:00 23:00:00
    Wait Until AV Plugin Log Contains  No of Exclusions Configured: 24
    Wait Until AV Plugin Log Contains  No of Sophos Defined Extension Exclusions Configured: 0
    Wait Until AV Plugin Log Contains  No of User Defined Extension Exclusions Configured: 0

AV Deletes Scan Correctly
    Check AV Plugin Installed With Base
    Send Complete Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    AV Plugin Log Contains  No of Scheduled Scans Configured: 1
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 11:00:00
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration
    Wait Until AV Plugin Log Contains  No of Scheduled Scans Configured: 0

Diagnose collects the correct files
    Check AV Plugin Installed With Base
    Configure and check scan now
    Run Diagnose
    Check Diagnose Tar Created
    Check Diagnose Collects Correct AV Files
    Check Diagnose Logs

AV Plugin Reports Threat XML To Base
   Check AV Plugin Installed With Base

   ${SCAN_DIRECTORY} =  Set Variable  /home/vagrant/this/is/a/directory/for/scanning

   Create File     ${SCAN_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output   /usr/local/bin/avscanner ${SCAN_DIRECTORY}/naugthy_eicar

   Log  ${output}

   Should Be Equal As Integers  ${rc}  69

   check threat event received by base  1   naugthyEicarThreatReport

AV Plugin uninstalls
    Check avscanner in /usr/local/bin
    Run plugin uninstaller
    Check avscanner not in /usr/local/bin


*** Keywords ***

Check avscanner in /usr/local/bin
    File Should Exist  /usr/local/bin/avscanner

Check avscanner not in /usr/local/bin
    File Should Not Exist  /usr/local/bin/avscanner

Run plugin uninstaller
    Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh

Configure and check scan now
    Configure scan now
    Check scan now

Configure scan now
    Send Sav Policy To Base  SAV_Policy_Scan_Now.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration

Check scan now
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains  Completed scan Scan Now
    AV Plugin Log Contains  Starting Scan Now scan
    AV Plugin Log Contains  Starting scan Scan Now

Validate latest Event
     ${eventXml}=  get_latest_xml_from_events  base/mcs/event/
     ${parsedXml}=  parse xml  ${eventXml}
     ELEMENT TEXT SHOULD MATCH  source=${parsedXml}  pattern=Scan Now  normalize_whitespace=True  xpath=scanComplete
