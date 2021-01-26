*** Settings ***
Documentation    Integration tests for AVP and Base
Default Tags  INTEGRATION
Library         Collections
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
Suite Teardown  Uninstall All

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Test Cases ***

AV plugin Can Start sophos_threat_detector
    Check AV Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check sophos_threat_detector Running

    Check Threat Detector Copied Files To Chroot

    Should Exist   ${CHROOT_LOGGING_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}/sophos_threat_detector.log

AV plugin runs scan now
    Check AV Plugin Installed With Base
    Configure and check scan now


AV plugin runs scan now while CLS is running
    Check AV Plugin Installed With Base
    Configure scan now

    #Scan something that should take a long time to scan
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /
    Send Sav Action To Base  ScanNow_Action.xml

    Wait Until AV Plugin Log Contains  Starting scan Scan Now  timeout=5
    Process Should Be Running   ${cls_handle}
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains  Sending scan complete
    ${result} =   Terminate Process  ${cls_handle}

AV plugin runs CLS while scan now is running
    [Teardown]  Run Keywords    AV And Base Teardown
    ...         AND             Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Check AV Plugin Installed With Base
    Configure scan now

    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh  stderr=STDOUT

    Remove file   ${SCANNOW_LOG_PATH}
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /tmp_test/three_hundred_eicars/
    Send Sav Action To Base  ScanNow_Action.xml

    Wait Until AV Plugin Log Contains  Starting scan Scan Now  timeout=5
    Process Should Be Running   ${cls_handle}
    Wait for Process    ${cls_handle}
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains  Sending scan complete
    List Directory   ${AV_PLUGIN_PATH}/log/
    File Log Contains  ${SCANNOW_LOG_PATH}  Attempting to scan mount point:
    Process Should Be Stopped   ${cls_handle}

AV plugin runs scan now twice consecutively
    Check AV Plugin Installed With Base
    Configure and check scan now
    Mark AV Log
    Check scan now

AV plugin attempts to run scan now twice simultaneously
    Check AV Plugin Installed With Base
    Mark AV Log
    Configure scan now

    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains  Starting scan Scan Now  timeout=5

    Send Sav Action To Base  ScanNow_Action.xml

    ## Wait for 1 scan to happen
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains  Sending scan complete

    ## Check we refused to start the second scan
    AV Plugin Log Contains  Refusing to run a second Scan named: Scan Now

    ## Check we started only one scan
    ${content} =  Get File   ${AV_LOG_PATH}  encoding_errors=replace
    ${lines} =  Get Lines Containing String     ${content}  Starting scan Scan Now

    ${count} =  Get Line Count   ${lines}
    Should Be Equal As Integers  ${1}  ${count}

AV plugin runs scheduled scan
    Check AV Plugin Installed With Base
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180

AV plugin runs multiple scheduled scans
    Check AV Plugin Installed With Base
    Send Sav Policy With Multiple Imminent Scheduled Scans To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains  Refusing to run a second Scan named: Sophos Cloud Scheduled Scan  timeout=120


AV plugin runs scheduled scan after restart
    Check AV Plugin Installed With Base
    Send Sav Policy With Imminent Scheduled Scan To Base
    Stop AV Plugin
    Remove File    ${AV_LOG_PATH}
    Start AV Plugin
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180

AV plugin fails scan now if no policy
    Check AV Plugin Installed With Base
    Send Sav Action To Base  ScanNow_Action.xml
    AV Plugin Log Does Not Contain  Starting scan scanNow
    AV Plugin Log Contains  Starting scan Scan Now

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
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_No_Scans.xml
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until Management Log Contains  Action SAV_action
    Wait Until AV Plugin Log Contains  Starting scan
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains  Sending scan complete
    Validate latest Event  ${now}

AV Gets SAV Policy When Plugin Restarts
    Check AV Plugin Installed With Base
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Stop AV Plugin
    Remove File    ${AV_LOG_PATH}
    Start AV Plugin
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 0

AV Gets ALC Policy When Plugin Restarts
    Check AV Plugin Installed With Base
    Send Alc Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/ALC-1_policy.xml
    Stop AV Plugin
    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Start AV Plugin
    Wait Until AV Plugin Log exists   timeout=30
    Wait Until AV Plugin Log Contains  Received policy from management agent for AppId: ALC

AV Configures No Scheduled Scan Correctly
    Check AV Plugin Installed With Base
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 0

AV plugin runs scheduled scan while CLS is running
    Check AV Plugin Installed With Base

    Send Sav Policy With Imminent Scheduled Scan To Base
    Wait Until AV Plugin Log Contains   Configured number of Scheduled Scans: 1

    #Scan something that should take ages to scan
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /

    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Process Should Be Running   ${cls_handle}
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180
    ${result} =   Terminate Process  ${cls_handle}

AV plugin runs CLS while scheduled scan is running
    [Teardown]  Run Keywords    AV And Base Teardown
        ...         AND             Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Check AV Plugin Installed With Base
    Send Sav Policy With Imminent Scheduled Scan To Base

    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh  stderr=STDOUT

    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /tmp_test/three_hundred_eicars/

    Process Should Be Running   ${cls_handle}
    Wait for Process    ${cls_handle}
    Wait Until AV Plugin Log Contains  Completed scan  timeout=180
    Process Should Be Stopped   ${cls_handle}

AV Configures Single Scheduled Scan Correctly
    Check AV Plugin Installed With Base
    Send Fixed Sav Policy
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

AV Handles Scheduled Scan With No Configured Day
    Check AV Plugin Installed With Base
    Mark Watchdog Log
    Send Sav Policy With No Scan Day
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Days: \n
    Wait Until AV Plugin Log Contains  Times: 11:00:00
    File Log Does Not Contain
    ...   Check Marked Watchdog Log Contains   av died

AV Handles Scheduled Scan With Badly Configured Time
    Check AV Plugin Installed With Base
    Send Sav Policy With Invalid Scan Time
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: 00:00:00

AV Handles Scheduled Scan With No Configured Time
    Check AV Plugin Installed With Base
    Mark Watchdog Log
    Send Sav Policy With No Scan Time
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains  Days: Monday
    Wait Until AV Plugin Log Contains  Times: \n
    File Log Does Not Contain
    ...   Check Marked Watchdog Log Contains   av died

AV Reconfigures Scans Correctly
    Check AV Plugin Installed With Base
    Send Fixed Sav Policy
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

   Log   ${output}

   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

   Wait Until Keyword Succeeds
         ...  60 secs
         ...  3 secs
         ...  check threat event received by base  1  naugthyEicarThreatReport

Avscanner runs as non-root
   Check AV Plugin Installed With Base

   ${SCAN_DIRECTORY} =  Set Variable  /home/vagrant/this/is/a/directory/for/scanning

   Create File     ${SCAN_DIRECTORY}/naugthy_eicar    ${EICAR_STRING}
   ${command} =    Set Variable    /usr/local/bin/avscanner ${SCAN_DIRECTORY}/naugthy_eicar
   ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
   ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

   Log   ${output}

   Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
   Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/naugthy_eicar" is infected with EICAR-AV-Test

   Should Not Contain    ${output}    Failed to read the config file
   Should Not Contain    ${output}    All settings will be set to their default value
   Should Contain   ${output}    Logger av configured for level: DEBUG

   Wait Until Keyword Succeeds
         ...  60 secs
         ...  3 secs
         ...  check threat event received by base  1  naugthyEicarThreatReportAsNobody

AV Plugin Reports encoded eicars To Base
   [Teardown]  Run Keywords      Remove Directory  /tmp_test/encoded_eicars  true
   ...         AND               AV And Base Teardown

   Check AV Plugin Installed With Base

   Create Encoded Eicars

   ${expected_count} =  Count Eicars in Directory  /tmp_test/encoded_eicars/
   Should Be True  ${expected_count} > 0

   ${result} =  Run Process  /usr/local/bin/avscanner  /tmp_test/encoded_eicars/  timeout=120s  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  ${VIRUS_DETECTED_RESULT}
   Log  ${result.stdout}

   #make sure base has generated all events before checking
   Wait Until Keyword Succeeds
         ...  60 secs
         ...  3 secs
         ...  check_number_of_events_matches  ${expected_count}

   check_multiple_different_threat_events  ${expected_count}   encoded_eicars

   Empty Directory  ${MCS_PATH}/event/


AV Plugin uninstalls
    Check avscanner in /usr/local/bin
    Run plugin uninstaller
    Check avscanner not in /usr/local/bin
    Check AV Plugin Not Installed
    [Teardown]   Install AV Directly from SDDS

AV Plugin Saves Logs On Downgrade
    Check AV Plugin Installed With Base
    Wait Until AV Plugin Log exists
    Wait Until AV Plugin Log Contains   av <> Starting threatDetector
    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed
    Check Logs Saved On Downgrade
    [Teardown]   Install AV Directly from SDDS

AV Plugin Can Send Telemetry
    Check AV Plugin Installed With Base
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     0
    Wait Until Keyword Succeeds
             ...  10 secs
             ...  1 secs
             ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV plugin Saves and Restores Scan Now Counter
    Check AV Plugin Installed With Base

    # Run telemetry to reset counters to 0
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}
    Remove File   ${TELEMETRY_OUTPUT_JSON}

    # run a scan, count should increase to 1
    Configure and check scan now

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   1

    Start AV Plugin

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   scan-now-count   1


AV plugin increments Scan Now Counter after Save and Restore
    Check AV Plugin Installed With Base

    # Run telemetry to reset counters to 0
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}
    Remove File   ${TELEMETRY_OUTPUT_JSON}

    # run a scan, count should increase to 1
    Configure and check scan now

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   1

    Start AV Plugin

    # run a scan, count should increase to 1
    Configure and check scan now

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   2
