*** Settings ***
Documentation    Integration tests for AVP and Base
Force Tags      INTEGRATION
Library         Collections
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
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
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check sophos_threat_detector Running

    Check Threat Detector Copied Files To Chroot

    Should Exist   ${CHROOT_LOGGING_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}/sophos_threat_detector.log

AV plugin runs scan now
    Configure and check scan now with offset


AV plugin runs scan now while CLS is running
    Configure scan now
    Mark AV Log

    #Scan something that should take a long time to scan
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /
    Send Sav Action To Base  ScanNow_Action.xml

    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5
    Process Should Be Running   ${cls_handle}
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete
    ${result} =   Terminate Process  ${cls_handle}

AV plugin runs CLS while scan now is running
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True
    Register Cleanup    Remove File  ${SCANNOW_LOG_PATH}

    Configure scan now
    Mark AV Log

    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh  stderr=STDOUT

    Remove file   ${SCANNOW_LOG_PATH}
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /tmp_test/three_hundred_eicars/
    Send Sav Action To Base  ScanNow_Action.xml

    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5
    Process Should Be Running   ${cls_handle}
    Wait for Process    ${cls_handle}
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete
    List Directory   ${AV_PLUGIN_PATH}/log/
    File Log Contains  ${SCANNOW_LOG_PATH}  Attempting to scan mount point:
    Process Should Be Stopped   ${cls_handle}

AV plugin runs scan now twice consecutively
    Configure and check scan now with offset
    Check scan now with Offset

AV plugin attempts to run scan now twice simultaneously
    Mark AV Log
    Configure scan now

    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5

    Send Sav Action To Base  ScanNow_Action.xml

    ## Wait for 1 scan to happen
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete

    ## Check we refused to start the second scan
    AV Plugin Log Contains With Offset  Refusing to run a second Scan named: Scan Now

    ## Check we started only one scan
    ${content} =  Get File Contents From Offset   ${AV_LOG_PATH}  ${AV_LOG_MARK}
    ${lines} =  Get Lines Containing String     ${content}  Starting scan Scan Now

    ${count} =  Get Line Count   ${lines}
    Should Be Equal As Integers  ${1}  ${count}

AV plugin runs scheduled scan
    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml

    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

AV plugin runs multiple scheduled scans
    Mark AV Log
    Send Sav Policy With Multiple Imminent Scheduled Scans To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains With Offset  Refusing to run a second Scan named: Sophos Cloud Scheduled Scan  timeout=120

AV plugin runs scheduled scan after restart
    Send Sav Policy With Imminent Scheduled Scan To Base
    Stop AV Plugin
    Mark AV Log
    Start AV Plugin
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

AV plugin fails scan now if no policy
    Stop AV Plugin
    Remove File     /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Start AV Plugin

    Wait until AV Plugin running

    Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Evaluating Scan Now
    AV Plugin Log Contains With Offset  Refusing to run invalid scan: INVALID

AV plugin SAV Status contains revision ID of policy
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini
    Send Sav Policy To Base  SAV_Policy.xml
    Wait Until SAV Status XML Contains  Res="Same"  timeout=60
    SAV Status XML Contains  RevID="ac9eaa2f09914ce947cfb14f1326b802ef0b9a86eca7f6c77557564e36dbff9a"
    SAV Status XML Contains  <product-version>${version}</product-version>

AV plugin sends Scan Complete event and (fake) Report To Central
    ${now} =  Get Current Date  result_format=epoch
    Mark AV Log
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_No_Scans.xml
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until Management Log Contains  Action SAV_action
    Wait Until AV Plugin Log Contains With Offset  Starting scan
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete
    Validate latest Event  ${now}

AV Gets SAV Policy When Plugin Restarts
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Stop AV Plugin
    Mark AV Log
    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  SAV policy received for the first time.
    Wait Until AV Plugin Log Contains With Offset  Processing SAV Policy
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 0

AV Gets ALC Policy When Plugin Restarts
    # Doesn't mark AV log since it removes it
    Send Alc Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/ALC-1_policy.xml
    Stop AV Plugin
    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Mark AV Log
    Mark Sophos Threat Detector Log
    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  ALC policy received for the first time.
    Wait Until AV Plugin Log Contains With Offset  Processing ALC Policy
    Threat Detector Log Should Not Contain With Offset   Failed to read customerID - using default value
    Wait Until AV Plugin Log Contains WIth Offset  Received policy from management agent for AppId: ALC

AV Configures No Scheduled Scan Correctly
    Mark AV Log
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 0

AV plugin runs scheduled scan while CLS is running
    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1

    #Scan something that should take ages to scan
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /

    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Process Should Be Running   ${cls_handle}
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    ${result} =   Terminate Process  ${cls_handle}

AV plugin runs CLS while scheduled scan is running
    [Teardown]  Run Keywords    AV And Base Teardown
        ...         AND             Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True
    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base

    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh  stderr=STDOUT

    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /tmp_test/three_hundred_eicars/

    Process Should Be Running   ${cls_handle}
    Wait for Process    ${cls_handle}
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Process Should Be Stopped   ${cls_handle}

AV Configures Single Scheduled Scan Correctly
    Mark AV Log
    Send Fixed Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00
    Wait Until AV Plugin Log Contains With Offset  Configured number of Exclusions: 28
    Wait Until AV Plugin Log Contains With Offset  Configured number of Sophos Defined Extension Exclusions: 3
    Wait Until AV Plugin Log Contains With Offset  Configured number of User Defined Extension Exclusions: 4

AV Configures Multiple Scheduled Scans Correctly
    Mark AV Log
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 2
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan One
    Wait Until AV Plugin Log Contains With Offset  Days: Tuesday Saturday
    Wait Until AV Plugin Log Contains With Offset  Times: 04:00:00 16:00:00
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan Two
    Wait Until AV Plugin Log Contains With Offset  Days: Monday Thursday
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00 23:00:00
    Wait Until AV Plugin Log Contains With Offset  Configured number of Exclusions: 25
    Wait Until AV Plugin Log Contains With Offset  Configured number of Sophos Defined Extension Exclusions: 0
    Wait Until AV Plugin Log Contains With Offset  Configured number of User Defined Extension Exclusions: 0

AV Handles Scheduled Scan With Badly Configured Day
    Mark AV Log
    Send Sav Policy With Invalid Scan Day
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Invalid day from policy: blernsday
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: INVALID
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00

AV Handles Scheduled Scan With No Configured Day
    Mark AV Log
    Mark Watchdog Log
    Send Sav Policy With No Scan Day
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: \n
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00
    File Log Does Not Contain
    ...   Check Marked Watchdog Log Contains   av died

AV Handles Scheduled Scan With Badly Configured Time
    Mark Av Log
    Send Sav Policy With Invalid Scan Time
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: 00:00:00

AV Handles Scheduled Scan With No Configured Time
    Mark Watchdog Log
    Mark AV Log
    Send Sav Policy With No Scan Time
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: \n
    File Log Does Not Contain
    ...   Check Marked Watchdog Log Contains   av died

AV Reconfigures Scans Correctly
    Mark AV Log
    Send Fixed Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00
    Wait Until AV Plugin Log Contains With Offset  Configured number of Exclusions: 28
    Wait Until AV Plugin Log Contains With Offset  Configured number of Sophos Defined Extension Exclusions: 3
    Wait Until AV Plugin Log Contains With Offset  Configured number of User Defined Extension Exclusions: 4
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 2
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan One
    Wait Until AV Plugin Log Contains With Offset  Days: Tuesday Saturday
    Wait Until AV Plugin Log Contains With Offset  Times: 04:00:00 16:00:00
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan Two
    Wait Until AV Plugin Log Contains With Offset  Days: Monday Thursday
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00 23:00:00
    Wait Until AV Plugin Log Contains With Offset  Configured number of Exclusions: 25
    Wait Until AV Plugin Log Contains With Offset  Configured number of Sophos Defined Extension Exclusions: 0
    Wait Until AV Plugin Log Contains With Offset  Configured number of User Defined Extension Exclusions: 0

AV Deletes Scan Correctly
    Mark AV Log
    Send Complete Sav Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00

    Mark AV Log
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 0

AV Plugin Reports Threat XML To Base
   Empty Directory  /opt/sophos-spl/base/mcs/event/
   Register Cleanup  Empty Directory  /opt/sophos-spl/base/mcs/event
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
   Empty Directory  /opt/sophos-spl/base/mcs/event/
   Register Cleanup  Empty Directory  /opt/sophos-spl/base/mcs/event/
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
    Check AV Plugin Running
    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed
    Check Logs Saved On Downgrade
    [Teardown]   Install AV Directly from SDDS

AV Plugin Can Send Telemetry
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
    # Run telemetry to reset counters to 0
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}
    Remove File   ${TELEMETRY_OUTPUT_JSON}

    # run a scan, count should increase to 1
    Configure and check scan now with offset

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
    # Run telemetry to reset counters to 0
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}
    Remove File   ${TELEMETRY_OUTPUT_JSON}

    # run a scan, count should increase to 1
    Configure and check scan now with offset

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
    Configure and check scan now with offset

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


AV Plugin Reports The Right Error Code If Sophos Threat Detector Dies During Scan Now
    Configure scan now
    Run Process  bash  ${BASH_SCRIPTS_PATH}/fileMaker.sh  1000  stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/file_maker/  recursive=True

    Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat

    Move File  ${SOPHOS_THREAT_DETECTOR_BINARY}.0  ${SOPHOS_THREAT_DETECTOR_BINARY}_moved
    Register Cleanup    Uninstall and full reinstall
    Run Process   /bin/kill   -SIGSEGV   ${output}

    Wait Until AV Plugin Log Contains With Offset  Scan: Scan Now, terminated with exit code: ${SCAN_ABORTED}   timeout=240    interval=5


AV Plugin Reports The Right Error Code If Sophos Threat Detector Dies During Scan Now With Threats
    Configure scan now
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat

    Move File  ${SOPHOS_THREAT_DETECTOR_BINARY}.0  $${SOPHOS_THREAT_DETECTOR_BINARY}_moved

    Wait Until AV Plugin Log Contains With Offset   Sending threat detection notification to central
    Register Cleanup    Uninstall and full reinstall
    Run Process   /bin/kill   -SIGSEGV   ${output}

    Wait Until AV Plugin Log Contains With Offset  Scan: Scan Now, found threats but aborted with exit code: ${SCAN_ABORTED_WITH_THREAT}    timeout=240    interval=5

AV Runs Scan With SXL Lookup Enable
    Mark AV Log
    Mark Susi Debug Log
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   stderr=STDOUT
    Configure and check scan now
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Wait Until AV Plugin Log Contains With Offset   Sending threat detection notification to central
    SUSI Debug Log Contains With Offset  Post-scan lookup succeeded


AV Runs Scan With SXL Lookup Disabled
    Mark AV Log
    Mark Susi Debug Log
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   stderr=STDOUT
    Configure and check scan now with lookups disabled
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Wait Until AV Plugin Log Contains With Offset  Sending threat detection notification to central
    SUSI Debug Log Does Not Contain With Offset   Post-scan lookup succeeded