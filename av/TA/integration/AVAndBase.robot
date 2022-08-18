*** Settings ***
Documentation    Integration tests for AVP and Base
Force Tags      INTEGRATION  AVBASE
Library         Collections
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/OSUtils.py
Library         ../Libs/ThreatReportUtils.py
Library         ../Libs/Telemetry.py

Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/AVAndBaseResources.robot

Suite Setup     AVAndBase Suite Setup
Suite Teardown  AVAndBase Suite Teardown

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Keywords ***
AVAndBase Suite Setup
    Install With Base SDDS
    # TODO: Remove stopping of soapd once file descriptor usage issue is fixed
    Stop soapd
    Send Alc Policy
    Send Sav Policy With No Scheduled Scans

AVAndBase Suite Teardown
    Uninstall All

Remove Users Stop Processes
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep av
    Run Process   /bin/kill   -SIGKILL   ${output}
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -SIGKILL   ${output}
    Run Process  /usr/sbin/userdel   sophos-spl-av
    Run Process  /usr/sbin/userdel   sophos-spl-threat-detector

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

    # Scan /usr/ with CLS (should take a long time)
    ${LOG_FILE} =       Set Variable   /tmp/scan.log
    Remove File  ${LOG_FILE}
    Register Cleanup    Remove File  ${LOG_FILE}
    Register Cleanup    Dump Log     ${LOG_FILE}
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /usr/
    ...   stdout=${LOG_FILE}   stderr=STDOUT
    Process Should Be Running   ${cls_handle}
    Register Cleanup    Terminate Process  ${cls_handle}

    # check CLS is scanning
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  File Log Contains  ${LOG_FILE}  Scanning

    # Start Scan Now
    Configure scan now
    Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    Mark Log   ${LOG_FILE}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  File Log Contains With Offset   ${LOG_FILE}   Scanning   ${LOG_MARK}

    # Wait for Scan Now to complete
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    Mark Log   ${LOG_FILE}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  File Log Contains With Offset   ${LOG_FILE}   Scanning   ${LOG_MARK}

    # Stop CLS
    ${result} =   Terminate Process  ${cls_handle}
    deregister cleanup  Terminate Process  ${cls_handle}

    Should Contain  ${{ [ ${EXECUTION_INTERRUPTED}, ${SCAN_ABORTED_WITH_THREAT} ] }}   ${result.rc}

AV plugin runs CLS while scan now is running

    # create something for scan now to work on
    Create Big Dir   count=60   path=/tmp_test/bigdir/

    # start scan now
    Configure scan now
    Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5

    # ensure avscanner is working
    check avscanner can detect eicar

    # check ScanNow is still scanning
    AV Plugin Log Should Not Contain With Offset   Completed scan

    # wait for scan now to complete
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete

AV plugin runs scan now twice consecutively
    Configure and check scan now with offset
    Check scan now with Offset

AV plugin attempts to run scan now twice simultaneously
    Register On Fail  dump log  ${SCANNOW_LOG_PATH}
    Mark AV Log
    Run Process  bash  ${BASH_SCRIPTS_PATH}/fileMaker.sh  1000  stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/file_maker/  recursive=True
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
    ${count} =   count lines in log with offset   ${AV_LOG_PATH}   Starting scan Scan Now   ${AV_LOG_MARK}
    Should Be Equal As Integers  ${1}  ${count}

AV plugin runs scheduled scan
    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180


AV plugin runs multiple scheduled scans
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude File Name Too Long For Cloud Scan
    Mark AV Log
    Register Cleanup  Restart AV Plugin And Clear The Logs For Integration Tests
    Send Sav Policy With Multiple Imminent Scheduled Scans To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains With Offset  Refusing to run a second Scan named: Sophos Cloud Scheduled Scan  timeout=120

AV plugin runs scheduled scan after restart
    Send Sav Policy With Imminent Scheduled Scan To Base
    Stop AV Plugin
    Mark AV Log
    Start AV Plugin
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Exclude Communication Between AV And Base Due To No Incoming Data

AV plugin reports an info message if no policy is received
    Stop AV Plugin
    Remove File     ${MCS_PATH}/policy/ALC-1_policy.xml
    Remove File     ${MCS_PATH}/policy/SAV-2_policy.xml

    Mark AV Log
    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  Failed to request SAV policy at startup (No Policy Available)
    Wait Until AV Plugin Log Contains With Offset  Failed to request ALC policy at startup (No Policy Available)

AV plugin fails scan now if no policy
    Register Cleanup    Exclude Scan As Invalid
    Stop AV Plugin
    Remove File     ${MCS_PATH}/policy/SAV-2_policy.xml
    Mark AV Log
    Mark Sophos Threat Detector Log
    Start AV Plugin

    Wait until AV Plugin running with offset

    Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Evaluating Scan Now
    AV Plugin Log Contains With Offset  Refusing to run invalid scan: INVALID

AV plugin SAV Status contains revision ID of policy
    Register Cleanup    Exclude Invalid Day From Policy
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
    register cleanup  remove file   ${MCS_PATH}/policy/SAV-2_policy.xml
    register cleanup  remove file   ${SUSI_STARTUP_SETTINGS_FILE}
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Stop AV Plugin
    Mark AV Log
    Mark Sophos Threat Detector Log

    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  SAV policy received for the first time.
    Wait Until AV Plugin Log Contains With Offset  Processing SAV Policy
    File Should Exist    ${SUSI_STARTUP_SETTINGS_FILE}
    File Should Exist    ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 0
    Scan GR Test File
    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be enabled

Av Plugin Processes First SAV Policy Correctly After Initial Wait For Policy Fails
    Stop AV Plugin
    Remove File    ${SUSI_STARTUP_SETTINGS_FILE}
    Remove File    ${MCS_PATH}/policy/SAV-2_policy.xml

    Mark AV Log
    Mark Sophos Threat Detector Log

    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset   SAV policy has not been sent to the plugin
    Send Sav Policy With No Scheduled Scans
    Wait Until AV Plugin Log Contains With Offset  Processing SAV Policy
    Wait Until File exists    ${SUSI_STARTUP_SETTINGS_FILE}
    File Should Exist    ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    Scan GR Test File
    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be enabled


AV Gets ALC Policy When Plugin Restarts
    Register Cleanup    Exclude UpdateScheduler Fails

    Send Alc Policy
    File Should Exist  ${MCS_PATH}/policy/ALC-1_policy.xml
    Stop AV Plugin

    Mark AV Log
    Mark Sophos Threat Detector Log
    Start AV Plugin
    Wait until AV Plugin running with offset
    Wait until threat detector running with offset

    Wait Until AV Plugin Log Contains With Offset  Received policy from management agent for AppId: ALC
    Wait Until AV Plugin Log Contains With Offset  ALC policy received for the first time.
    Wait Until AV Plugin Log Contains With Offset  Processing ALC Policy

    Threat Detector Log Should Not Contain With Offset  Failed to read customerID - using default value

AV Configures No Scheduled Scan Correctly
    Register Cleanup    Exclude UpdateScheduler Fails
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    Mark AV Log
    Send Sav Policy With No Scheduled Scans
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 0

AV plugin runs scheduled scan while CLS is running
    #Terminate Process might cause this error
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    Register Cleanup    Exclude Scan Errors From File Samples

    Create File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector
    Register Cleanup  Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

    # Scan /usr/ with CLS (should take a long time)
    ${LOG_FILE} =       Set Variable   /tmp/scan.log
    Remove File  ${LOG_FILE}
    Register Cleanup    Remove File  ${LOG_FILE}
    Register Cleanup    Dump Log     ${LOG_FILE}
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /usr/
    ...   stdout=${LOG_FILE}   stderr=STDOUT
    Process Should Be Running   ${cls_handle}

    # check CLS is scanning
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  File Log Contains  ${LOG_FILE}  Scanning

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    Mark Log   ${LOG_FILE}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains With Offset   ${LOG_FILE}   Scanning   ${LOG_MARK}

    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    Mark Log   ${LOG_FILE}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Log Contains With Offset   ${LOG_FILE}   Scanning   ${LOG_MARK}

    # Stop CLS
    ${result} =   Terminate Process  ${cls_handle}
    Should Contain  ${{ [ ${EXECUTION_INTERRUPTED}, ${SCAN_ABORTED_WITH_THREAT} ] }}   ${result.rc}

AV plugin runs CLS while scheduled scan is running
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base

    # create something for scheduled scan to work on
    Create Big Dir   count=60   path=/tmp_test/bigdir/

    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=150

    check avscanner can detect eicar

    AV Plugin Log Should Not Contain With Offset   Completed scan
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

AV Configures Single Scheduled Scan Correctly
    Mark AV Log
    Send Fixed Sav Policy
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
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
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
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
    Register Cleanup    Exclude Invalid Day From Policy
    Register Cleanup    Exclude Scan As Invalid
    Mark AV Log
    Send Sav Policy With Invalid Scan Day
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Invalid day from policy: blernsday
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: INVALID
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00

AV Handles Scheduled Scan With No Configured Day
    Register Cleanup    Exclude Invalid Day From Policy
    Mark AV Log
    Mark Watchdog Log
    Send Sav Policy With No Scan Day
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: \n
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00
    File Log Does Not Contain
    ...   Check Marked Watchdog Log Contains   av died

AV Handles Scheduled Scan With Badly Configured Time
    Mark Av Log
    Send Sav Policy With Invalid Scan Time
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: 00:00:00

AV Handles Scheduled Scan With No Configured Time
    Mark Watchdog Log
    Mark AV Log
    Send Sav Policy With No Scan Time
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: \n
    File Log Does Not Contain
    ...   Check Marked Watchdog Log Contains   av died

AV Reconfigures Scans Correctly
    Mark AV Log
    Send Fixed Sav Policy
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00
    Wait Until AV Plugin Log Contains With Offset  Configured number of Exclusions: 28
    Wait Until AV Plugin Log Contains With Offset  Configured number of Sophos Defined Extension Exclusions: 3
    Wait Until AV Plugin Log Contains With Offset  Configured number of User Defined Extension Exclusions: 4
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
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
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: Sophos Cloud Scheduled Scan
    Wait Until AV Plugin Log Contains With Offset  Days: Monday
    Wait Until AV Plugin Log Contains With Offset  Times: 11:00:00

    Mark AV Log
    Send Sav Policy With No Scheduled Scans
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 0

AV Plugin Reports Threat XML To Base
   Empty Directory  ${MCS_PATH}/event/
   Register Cleanup  Empty Directory  ${MCS_PATH}/event
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
   Empty Directory  ${MCS_PATH}/event/
   Register Cleanup  Empty Directory  ${MCS_PATH}/event/
   Register Cleanup  List Directory  ${MCS_PATH}/event/

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
   Register Cleanup  Exclude Failed To connect To Warehouse Error
   Create Encoded Eicars
   register cleanup  Remove Directory  /tmp_test/encoded_eicars  true

   # TODO - assumes no other MCS events
   Empty Directory  ${MCS_PATH}/event/
   Register Cleanup  Empty Directory  ${MCS_PATH}/event/
   Register Cleanup  List Directory  ${MCS_PATH}/event/

   ${expected_count} =  Count Eicars in Directory  /tmp_test/encoded_eicars/
   Should Be True  ${expected_count} > 0

   ${result} =  Run Process  /usr/local/bin/avscanner  /tmp_test/encoded_eicars/  timeout=120s  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  ${VIRUS_DETECTED_RESULT}
   Log  ${result.stdout}

   #make sure base has generated all events before checking
   Wait Until Keyword Succeeds
         ...  60 secs
         ...  5 secs
         ...  check_number_of_threat_events_matches  ${expected_count}

   check_all_eicars_are_found  /tmp_test/encoded_eicars/

AV Plugin uninstalls
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Install With Base SDDS
    Check avscanner in /usr/local/bin
    Run plugin uninstaller
    #LINUXDAR-3861. Remove when this ticket has been closed
    Remove Users Stop Processes
    #end
    Check avscanner not in /usr/local/bin
    Check AV Plugin Not Installed

AV Plugin Saves Logs On Downgrade
    Register Cleanup  Exclude MCS Router is dead
    Register Cleanup  Install With Base SDDS
    Check AV Plugin Running
    Run plugin uninstaller with downgrade flag
    #LINUXDAR-3861. Remove when this ticket has been closed
    Remove Users Stop Processes
    #end
    Check AV Plugin Not Installed
    Check Logs Saved On Downgrade

AV Plugin Reports The Right Error Code If Sophos Threat Detector Dies During Scan Now
    [Timeout]  15min
    ignore coredumps and segfaults
    #We do a full uninstall so after the re-installation exclude this error
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude Threat Detector Launcher Died With 70
    Register Cleanup    Exclude Threat Detector Launcher Died With 11
    Register Cleanup    Exclude Scan Now Terminated
    Register Cleanup    Exclude Failed To Scan Files
    Register Cleanup    Exclude Failed To Send Scan Request To STD
    Register On Fail    dump log  ${SCANNOW_LOG_PATH}
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

    Wait Until AV Plugin Log Contains With Offset  Scan: Scan Now, terminated with exit code: ${SCAN_ABORTED}
    ...  timeout=${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD}    interval=10

AV Plugin Reports The Right Error Code If Sophos Threat Detector Dies During Scan Now With Threats
    [Timeout]  15min
    ignore coredumps and segfaults
    register cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude Threat Detector Launcher Died With 70
    Register Cleanup    Exclude Threat Detector Launcher Died With 11
    Register Cleanup    Exclude Scan Now Terminated
    Register Cleanup    Exclude Failed To Scan Files
    Register Cleanup    Exclude Failed To Send Scan Request To STD
    Register Cleanup    Exclude Scan Now Found Threats But Aborted With 25
    Configure scan now
    Start Sophos Threat Detector if not running
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   /tmp_test/many_eicars  600  stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/many_eicars  recursive=True

    Mark AV Log
    Register Cleanup    Remove File  ${SCANNOW_LOG_PATH}
    Remove File  ${SCANNOW_LOG_PATH}
    Register On Fail  dump log  ${SCANNOW_LOG_PATH}

    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=5
    ## Scan only takes ~3 seconds once scanning starts, so we need to finish this quickly
    Wait Until AV Plugin Log Contains With Offset  Sending threat detection notification to central  timeout=60  interval=1

    Move File  ${SOPHOS_THREAT_DETECTOR_BINARY}.0  ${SOPHOS_THREAT_DETECTOR_BINARY}_moved
    Register Cleanup    Uninstall and full reinstall
    ${pid} =   Record Sophos Threat Detector PID
    Run Process   /bin/kill   -SIGSEGV   ${pid}

    Wait Until AV Plugin Log Contains With Offset  Scan: Scan Now, found threats but aborted with exit code: ${SCAN_ABORTED_WITH_THREAT}
    ...  timeout=${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD}    interval=20

AV Runs Scan With SXL Lookup Enabled
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Mark Susi Debug Log
    Configure and check scan now with offset
    Wait Until AV Plugin Log Contains With Offset   Sending threat detection notification to central   timeout=60
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now

    SUSI Debug Log Contains With Offset  Post-scan lookup succeeded


AV Runs Scan With SXL Lookup Disabled
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Mark AV Log
    Mark Susi Debug Log
    Mark Sophos Threat Detector Log

    Configure and check scan now with lookups disabled

    Wait Until AV Plugin Log Contains With Offset  Sending threat detection notification to central   timeout=60
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now

    SUSI Debug Log Does Not Contain With Offset   Post-scan lookup started
    SUSI Debug Log Does Not Contain With Offset   Post-scan lookup succeeded
    AV Plugin Log Does Not Contain   Failed to send shutdown request: Failed to connect to unix socket


AV Plugin does not restart threat detector on customer id change
    #    Get ALC Policy doesn't have Core and Base
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Configuration Data Invalid
    Register Cleanup    Exclude Invalid Settings No Primary Product
    ${pid} =   Record Sophos Threat Detector PID

    # scan eicar to ensure susi is loaded, so that we know which log messages to expect later
    Check avscanner can detect eicar

    ${id1} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id1}  userpassword=${id1}  username=${id1}
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}

    Mark AV Log
    Mark Sophos Threat Detector Log

    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Reloading susi as policy configuration has changed
    #Wait Until Sophos Threat Detector Log Contains With Offset   Skipping susi reload because susi is not initialised
    Wait Until Sophos Threat Detector Log Contains With Offset   Susi configuration reloaded
    Check Sophos Threat Detector has same PID   ${pid}

    # change revid only, threat_detector should not restart
    ${pid} =   Record Sophos Threat Detector PID

    ${id2} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id2}  userpassword=${id1}  username=${id1}
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}
    Mark AV Log
    Mark Sophos Threat Detector Log
    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Run Keyword And Expect Error
    ...   Keyword 'AV Plugin Log Contains With Offset' failed after retrying for 5 seconds.*
    ...   Wait Until AV Plugin Log Contains With Offset   Reloading susi as policy configuration has changed   timeout=5
    Check Sophos Threat Detector has same PID   ${pid}

    # change credentials, avp should issue not a susi reload request
    ${pid} =   Record Sophos Threat Detector PID

    ${id3} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id3}  userpassword=${id3}  username=${id3}
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}

    Mark AV Log
    Mark Sophos Threat Detector Log
    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Reloading susi as policy configuration has changed
    #Wait Until Sophos Threat Detector Log Contains With Offset   Skipping susi reload because susi is not initialised
    Wait Until Sophos Threat Detector Log Contains With Offset   Susi configuration reloaded
    Check Sophos Threat Detector has same PID   ${pid}


AV Plugin tries to restart threat detector on susi startup settings change
    Register Cleanup    Exclude Threat Detector Launcher Died
    Register Cleanup    Exclude Failed To Write To UnixSocket Environment Interuption
    Register Cleanup    Exclude Invalid Settings No Primary Product
    Register Cleanup    Exclude Configuration Data Invalid

    Comment  set our initial policy

    ${revid} =   Generate Random String
    ${policyContent} =   Get SAV Policy  revid=${revid}  sxlLookupEnabled=true
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml
    Wait Until SAV Status XML Contains  RevID="${revid}"

    Restart sophos_threat_detector and mark logs
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Process Controller Server starting listening on socket: /var/process_control_socket
    ...   timeout=60
    stop sophos_threat_detector

    Comment  disable SXL lookups, AV should try to reload SUSI

    Mark AV Log
    Mark Sophos Threat Detector Log

    ${revid} =   Generate Random String
    ${policyContent} =   Get SAV Policy  revid=${revid}  sxlLookupEnabled=false
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Reloading susi as policy configuration has changed   timeout=60
    AV Plugin Log Contains With Offset  Failed to connect to Sophos Threat Detector Controller - retrying after sleep
    Wait Until AV Plugin Log Contains With Offset  Reached total maximum number of connection attempts.

    start sophos_threat_detector
    Wait until threat detector running with offset

    Comment  change lookup setting, threat_detector should reload SUSI

    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${revid} =   Generate Random String
    ${policyContent} =   Get SAV Policy  revid=${revid}  sxlLookupEnabled=true
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Reloading susi as policy configuration has changed   timeout=60
    AV Plugin Log Does Not Contain With Offset  Failed to send shutdown request: Failed to connect to unix socket
    #AV Plugin Log Does Not Contain With Offset  Failed to connect to Sophos Threat Detector Controller - retrying after sleep
    Check Sophos Threat Detector has same PID   ${pid}

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be enabled   timeout=180

Sophos Threat Detector sets default if susi startup settings permissions incorrect
    [Tags]  FAULT INJECTION
    Register Cleanup    Exclude Configuration Data Invalid
    Register Cleanup    Exclude Invalid Settings No Primary Product

    Restart sophos_threat_detector and mark logs
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Process Controller Server starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Mark Sophos Threat Detector Log

    Mark AV Log
    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml
    Wait Until AV Plugin Log Contains With Offset   Received new policy

    # TODO - fails if SXL was already disabled (e.g. when run after "AV Runs Scan With SXL Lookup Disabled")
    Wait Until Sophos Threat Detector Log Contains With Offset  Skipping susi reload because susi is not initialised

    Run Process  chmod  000  ${SUSI_STARTUP_SETTINGS_FILE}
    Run Process  chmod  000  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}
    Register Cleanup   Remove File   ${SUSI_STARTUP_SETTINGS_FILE}
    Register Cleanup   Remove File   ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset   Turning Live Protection on as default - no susi startup settings found

AV Plugin Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Empty Directory  ${MCS_PATH}/event/
    Register Cleanup    List Directory  ${MCS_PATH}/event/
    Empty Directory    ${MCS_PATH}/event/

    Create File  ${NORMAL_DIRECTORY}/naugthy_eicar  ${EICAR_STRING}
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/naugthy_eicar

    Mark AV Log
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naugthy_eicar

    # Verify that we get a log message for the eicar
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naugthy_eicar'"

    # Verify that the AV Plugin sends an alert when logging is working
    Wait Until Keyword Succeeds
       ...  60 secs
       ...  5 secs
       ...  check threat event received by base  1  naugthyEicarThreatReport

    Empty Directory  ${MCS_PATH}/event/

    Run  chmod 444 ${AV_LOG_PATH}
    Register Cleanup  Stop AV Plugin
    Register Cleanup  Run  chmod 600 ${AV_LOG_PATH}

    ${INITIAL_AV_PID} =  Record AV Plugin PID
    Log  Initial PID: ${INITIAL_AV_PID}
    Stop AV Plugin
    # Ensure threat reporting socket is deleted before restarting AV Plugin
    Remove File  ${COMPONENT_ROOT_PATH}/chroot/var/threat_report_socket
    Start AV Plugin
    ${END_AV_PID} =  Record AV Plugin PID
    Log  Restarted PID: ${END_AV_PID}
    # Verify the restart actually happened
    Should Not Be Equal As Integers  ${INITIAL_AV_PID}  ${END_AV_PID}

    # Can't verify reporting socket is ready from logs so just have to wait:
    wait until created  ${COMPONENT_ROOT_PATH}/chroot/var/threat_report_socket
    Sleep  5s  wait for av process to start up

    Mark AV Log

    ${result} =  Run Process  ls  -l  ${AV_LOG_PATH}
    Log  New permissions: ${result.stdout}

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naugthy_eicar

    # Verify the av plugin still sent the alert
    Wait Until Keyword Succeeds
        ...  60 secs
        ...  5 secs
        ...  check threat event received by base  1  naugthyEicarThreatReport

    # Verify that the log wasn't written (permission blocked)
    AV Plugin Log Should Not Contain With Offset  <notification description="Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naugthy_eicar'"

Scan Now Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Remove File  ${SCANNOW_LOG_PATH}
    Remove File  ${SCANNOW_LOG_PATH}

    Create File  /tmp_test/naughty_eicar  ${EICAR_STRING}
    Register Cleanup    Remove File  /tmp_test/naughty_eicar

    Configure scan now
    Mark AV Log

    Send Sav Action To Base  ScanNow_Action.xml

    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=40
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete
    Log File  ${SCANNOW_LOG_PATH}
    File Log Contains  ${SCANNOW_LOG_PATH}  Detected "/tmp_test/naughty_eicar" is infected with EICAR-AV-Test
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '/tmp_test/naughty_eicar'"

    ${result} =  Run Process  ls  -l  ${SCANNOW_LOG_PATH}
    Log  Old permissions: ${result.stdout}

    Mark Scan Now Log

    Run  chmod 444 '${SCANNOW_LOG_PATH}'
    Register Cleanup  Run  chmod 600 '${SCANNOW_LOG_PATH}'

    ${result} =  Run Process  ls  -l  ${SCANNOW_LOG_PATH}
    Log  New permissions: ${result.stdout}

    Configure scan now
    Mark AV Log

    Send Sav Action To Base  ScanNow_Action.xml

    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=40
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Sending scan complete
    Log File  ${SCANNOW_LOG_PATH}
    File Log Should Not Contain With Offset  ${SCANNOW_LOG_PATH}  Detected "${NORMAL_DIRECTORY}/naughty_eicar" is infected with EICAR-AV-Test  ${SCAN_NOW_LOG_MARK}
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '/tmp_test/naughty_eicar'"

Scheduled Scan Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    Register Cleanup    Remove File  ${CLOUDSCAN_LOG_PATH}
    Register Cleanup    Exclude Failed To Write To UnixSocket Environment Interuption

    Remove File  ${CLOUDSCAN_LOG_PATH}

    Create File  /tmp_test/naughty_eicar  ${EICAR_STRING}
    Register Cleanup    Remove File  /tmp_test/naughty_eicar

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=250
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=18
    Log File  ${CLOUDSCAN_LOG_PATH}
    File Log Contains  ${CLOUDSCAN_LOG_PATH}  Detected "/tmp_test/naughty_eicar" is infected with EICAR-AV-Test

    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '/tmp_test/naughty_eicar'"

    ${result} =  Run Process  ls  -l  ${CLOUDSCAN_LOG_PATH}
    Log  Old permissions: ${result.stdout}

    Mark Log  ${CLOUDSCAN_LOG_PATH}

    Run  chmod 444 '${CLOUDSCAN_LOG_PATH}'
    Register Cleanup  Run  chmod 600 '${CLOUDSCAN_LOG_PATH}'

    ${result} =  Run Process  ls  -l  ${CLOUDSCAN_LOG_PATH}
    Log  New permissions: ${result.stdout}

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=250
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=18
    Log File  ${CLOUDSCAN_LOG_PATH}
    File Log Should Not Contain With Offset  ${CLOUDSCAN_LOG_PATH}  Detected "${NORMAL_DIRECTORY}/naughty_eicar" is infected with EICAR-AV-Test  ${LOG_MARK}
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '/tmp_test/naughty_eicar'"

On Access Logs When A File Is Closed Following Write
    Mark On Access Log

    Start soapd
    Register cleanup  Stop soapd
    Send Sav Policy To Base  SAV-2_policy_OA_enabled.xml
    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until On Access Log Contains  Starting eventReader
    ${pid} =  Get Robot Pid
    Create File  /tmp_test/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  /tmp_test/eicar.com
    Wait Until On Access Log Contains  On-close event from PID ${pid} for FD