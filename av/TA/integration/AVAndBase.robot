*** Settings ***
Documentation    Integration tests for AVP and Base
Default Tags  INTEGRATION
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ThreatReportUtils.py

Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/AVAndBaseResources.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      AV And Base Setup
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

AV plugin fails scan now if no policy
    Check AV Plugin Installed With Base
    Send Sav Action To Base  ScanNow_Action.xml
    AV Plugin Log Does Not Contain  Starting scan scanNow
    AV Plugin Log Contains  Starting Scan Now

AV plugin SAV Status contains revision ID of policy
    Check AV Plugin Installed With Base
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini
    Send Sav Policy To Base  SAV_Policy.xml
    Wait Until SAV Status XML Contains  Res="Same"  timeout=60
    SAV Status XML Contains  RevID="ac9eaa2f09914ce947cfb14f1326b802ef0b9a86eca7f6c77557564e36dbff9a"
    SAV Status XML Contains  <product-version>${version}</product-version>

AV plugin sends Scan Complete event and (fake) Report To Central
    Check AV Plugin Installed With Base
    ${now} =  Get Current Date  result_format=epoch
    Send Sav Policy To Base  SAV_Policy.xml
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until Management Log Contains  Action /opt/sophos-spl/base/mcs/action/SAV_action
    Wait Until AV Plugin Log Contains  Starting scan
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains  Sending scan complete
    Validate latest Event  ${now}

AV Configures No Scheduled Scan Correctly
    Check AV Plugin Installed With Base
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 0

AV Configures Single Scheduled Scan Correctly
    Check AV Plugin Installed With Base
    Send Complete Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 11:00:00
    Wait Until AV Plugin Log Contains  Configured number of Exclusions: 28
    Wait Until AV Plugin Log Contains  Configured number of Sophos Defined Extension Exclusions: 3
    Wait Until AV Plugin Log Contains  Configured number of User Defined Extension Exclusions: 4

AV Configures Multiple Scheduled Scans Correctly
    Check AV Plugin Installed With Base
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 2
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan One
    Wait Until AV Plugin Log Contains  Days: Tuesday Saturday
    Wait Until AV Plugin Log Contains  Times: 04:00:00 16:00:00
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan Two
    Wait Until AV Plugin Log Contains  Days: Monday Thursday
    Wait Until AV Plugin Log Contains  Times: 11:00:00 23:00:00
    Wait Until AV Plugin Log Contains  Configured number of Exclusions: 25
    Wait Until AV Plugin Log Contains  Configured number of Sophos Defined Extension Exclusions: 0
    Wait Until AV Plugin Log Contains  Configured number of User Defined Extension Exclusions: 0

AV Handles Scheduled Scan With Badly Configured Day
    Check AV Plugin Installed With Base
    Send Sav Policy With Invalid Scan Day
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Invalid day from policy: blernsday
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Days: INVALID
    Wait Until AV Plugin Log Contains  Times: 11:00:00

AV Handles Scheduled Scan With Badly Configured Time
    Check AV Plugin Installed With Base
    Send Sav Policy With Invalid Scan Time
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 00:00:00

AV Reconfigures Scans Correctly
    Check AV Plugin Installed With Base
    Send Complete Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 11:00:00
    Wait Until AV Plugin Log Contains  Configured number of Exclusions: 28
    Wait Until AV Plugin Log Contains  Configured number of Sophos Defined Extension Exclusions: 3
    Wait Until AV Plugin Log Contains  Configured number of User Defined Extension Exclusions: 4
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 2
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan One
    Wait Until AV Plugin Log Contains  Days: Tuesday Saturday
    Wait Until AV Plugin Log Contains  Times: 04:00:00 16:00:00
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan Two
    Wait Until AV Plugin Log Contains  Days: Monday Thursday
    Wait Until AV Plugin Log Contains  Times: 11:00:00 23:00:00
    Wait Until AV Plugin Log Contains  Configured number of Exclusions: 25
    Wait Until AV Plugin Log Contains  Configured number of Sophos Defined Extension Exclusions: 0
    Wait Until AV Plugin Log Contains  Configured number of User Defined Extension Exclusions: 0

AV Deletes Scan Correctly
    Check AV Plugin Installed With Base
    Send Complete Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 11:00:00
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 0

AV Plugin Reports Threat XML To Base
   Check AV Plugin Installed With Base

   ${SCAN_DIRECTORY} =  Set Variable  /home/vagrant/this/is/a/directory/for/scanning

   Create File     ${SCAN_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   ${rc}   ${output} =    Run And Return Rc And Output   /usr/local/bin/avscanner ${SCAN_DIRECTORY}/naugthy_eicar

   Log To Console  ${output}

   Should Be Equal As Integers  ${rc}  69

   check threat event received by base  1   naugthyEicarThreatReport

AV Plugin Reports encoded eicars To Base
   Check AV Plugin Installed With Base

   ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  0
   Log  ${result.stdout}

   ${expected_count} =  Count Eicars in Directory  /tmp/encoded_eicars/
   Should Be True  ${expected_count} > 0

   ${result} =  Run Process  /usr/local/bin/avscanner  /tmp/encoded_eicars/  timeout=120s  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  69
   Log  ${result.stdout}


   #make sure base has generated all events before checking
   Wait Until Keyword Succeeds
         ...  15 secs
         ...  3 secs
         ...  check_number_of_events_matches  ${expected_count}

   check_multiple_different_threat_events  ${expected_count}   encoded_eicars

   Empty Directory  ${MCS_PATH}/event/
   Remove Directory  /tmp/encoded_eicars  true

AV Plugin uninstalls
    Check avscanner in /usr/local/bin
    Run plugin uninstaller
    Check avscanner not in /usr/local/bin
    Check AV Plugin Not Installed
    [Teardown]   Install AV Directly from SDDS

AV Plugin Can Send Telemetry
    Check AV Plugin Installed With Base
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     0
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av
    Should Contain   ${telemetryFileContents}   av
    Should Contain   ${telemetryFileContents}   version

