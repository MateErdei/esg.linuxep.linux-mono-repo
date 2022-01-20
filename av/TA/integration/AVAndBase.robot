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
Library         ../Libs/Telemetry.py

Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/AVAndBaseResources.robot

Suite Setup     AVAndBase Suite Setup
Suite Teardown  Uninstall All

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Keywords ***
AVAndBase Suite Setup
    Install With Base SDDS

Log Telemetry files
    ${result} =  Run Process  ls -ld ${SSPL_BASE}/bin/telemetry ${SSPL_BASE}/bin ${SSPL_BASE}/bin/telemetry.*  shell=True  stdout=/tmp/telemetry.files  stderr=STDOUT
    Log  Telemetry files: ${result.stdout} code ${result.rc}
    Debug Telemetry  ${SSPL_BASE}/bin/telemetry

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
    Register Cleanup    Exclude UnixSocket Failed To Read Length
    Register Cleanup    Exclude UnixSocket Connection Reset By Peer
    Register Cleanup    Exclude UnixSocket Environment Interruption Error
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
    Register Cleanup    Exclude UnixSocket Environment Interruption Error
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True
    Register Cleanup    Remove File  ${SCANNOW_LOG_PATH}

    Configure scan now
    Mark AV Log

    Run Process  bash  ${BASH_SCRIPTS_PATH}/fakeEicarMaker.sh  stderr=STDOUT

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

AV plugin runs scheduled scan and updates telemetry
    [Tags]  SLOW  RHEL7  TELEMETRY
    # Run telemetry to reset counters to 0
    Register On Fail  Log Telemetry files
    Prepare To Run Telemetry Executable With HTTPS Protocol  port=${4421}

    Log Telemetry files
    ${result} =  Run Process  sudo  -u  sophos-spl-user  id  stderr=STDOUT
    Log  "id sophos-spl-user = ${result.stdout}"

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                ...  10 secs
                ...  1 secs
                ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}
    Remove File   ${TELEMETRY_OUTPUT_JSON}

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml

    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

    Prepare To Run Telemetry Executable With HTTPS Protocol  port=${4421}
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   1


AV plugin runs multiple scheduled scans
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude File Name Too Long For Cloud Scan
    Mark AV Log
    Register Cleanup  Restart AV Plugin And Clear The Logs For Integration Tests
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

AV plugin doesnt report a error message if no policy is received
    register cleanup  Set Log Level  DEBUG
    Mark AV Log
    Stop AV Plugin
    Remove File     /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml

    Set Log Level  ERROR
    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset   Logger av configured for level
    Sleep  5  #Giving a chance for the plugin to request policy
    AV Plugin Log Does Not Contain With Offset  Failed to get SAV policy at startup (No Policy Available)
    AV Plugin Log Does Not Contain With Offset  Failed to get ALC policy at startup (No Policy Available)

AV plugin does report a info message if no policy is received
    Mark AV Log
    Stop AV Plugin
    Remove File     /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml

    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  Failed to get SAV policy at startup (No Policy Available)
    Wait Until AV Plugin Log Contains With Offset  Failed to get ALC policy at startup (No Policy Available)

AV plugin fails scan now if no policy
    Register Cleanup    Exclude Scan As Invalid
    Stop AV Plugin
    Remove File     /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
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
    # Doesn't mark AV log since it removes it
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Stop AV Plugin
    Remove File    ${AV_LOG_PATH}
    Start AV Plugin
    Wait Until AV Plugin Log Contains  SAV policy received for the first time.
    Wait Until AV Plugin Log Contains  Processing SAV Policy
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans: 0

AV Gets ALC Policy When Plugin Restarts
    Register Cleanup    Exclude UpdateScheduler Fails
    # Doesn't mark AV log since it removes it
    Send Alc Policy
    File Should Exist  /opt/sophos-spl/base/mcs/policy/ALC-1_policy.xml
    Stop AV Plugin
    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Start AV Plugin
    Wait Until AV Plugin Log exists   timeout=30
    Wait Until Threat Detector Log exists
    Wait Until AV Plugin Log Contains  ALC policy received for the first time.
    Wait Until AV Plugin Log Contains  Processing ALC Policy
    Threat Detector Does Not Log Contain  Failed to read customerID - using default value
    Wait Until AV Plugin Log Contains  Received policy from management agent for AppId: ALC

AV Configures No Scheduled Scan Correctly
    Register Cleanup    Exclude UpdateScheduler Fails
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    Mark AV Log
    Send Sav Policy With No Scheduled Scans
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans: 0

AV plugin runs scheduled scan while CLS is running
    #Terminate Process might cause this error
    Register Cleanup    Exclude UnixSocket Environment Interruption Error
    Register Cleanup    Exclude UnixSocket Connection Reset By Peer
    Register Cleanup    Exclude Failed To connect To Warehouse Error
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
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True
    Register Cleanup    Exclude UnixSocket Environment Interruption Error
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base

    Run Process  bash  ${BASH_SCRIPTS_PATH}/fakeEicarMaker.sh  stderr=STDOUT

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
    Register Cleanup    Exclude Invalid Day From Policy
    Register Cleanup    Exclude Scan As Invalid
    Mark AV Log
    Send Sav Policy With Invalid Scan Day
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
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
   Register Cleanup  Exclude Failed To connect To Warehouse Error
   Create Encoded Eicars
   register cleanup  Remove Directory  /tmp_test/encoded_eicars  true
   register cleanup  Empty Directory  ${MCS_PATH}/event/

   ${expected_count} =  Count Eicars in Directory  /tmp_test/encoded_eicars/
   Should Be True  ${expected_count} > 0

   ${result} =  Run Process  /usr/local/bin/avscanner  /tmp_test/encoded_eicars/  timeout=120s  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  ${VIRUS_DETECTED_RESULT}
   Log  ${result.stdout}

   #make sure base has generated all events before checking
   Wait Until Keyword Succeeds
         ...  60 secs
         ...  5 secs
         ...  check_number_of_events_matches  ${expected_count}

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

AV Plugin Can Send Telemetry
    Prepare To Run Telemetry Executable With HTTPS Protocol  port=${4431}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     0
    Wait Until Keyword Succeeds
             ...  10 secs
             ...  1 secs
             ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV Plugin sends non-zero processInfo to Telemetry
    Prepare To Run Telemetry Executable With HTTPS Protocol  port=${4432}
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    ${memUsage}=    Get From Dictionary   ${avDict}   threatMemoryUsage
    ${processAge}=    Get From Dictionary   ${avDict}   threatProcessAge

    Should Not Be Equal As Integers  ${memUsage}  0
    Should Not Be Equal As Integers  ${processAge}  0

AV plugin Saves and Restores Scan Now Counter
    # Run telemetry to reset counters to 0
    Prepare To Run Telemetry Executable With HTTPS Protocol  port=${4433}
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
    Dictionary Should Contain Item   ${rootkeyDict}   threatHealth   1

    Mark AV Log
    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  Restoring telemetry from disk for plugin: av
    Prepare To Run Telemetry Executable With HTTPS Protocol  port=${4434}
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
    Dictionary Should Contain Item   ${avDict}   threatHealth   1


AV plugin increments Scan Now Counter after Save and Restore
    # Run telemetry to reset counters to 0
    Prepare To Run Telemetry Executable With HTTPS Protocol  port=${4435}
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
    [Timeout]  15min
    #We do a full uninstall so after the re-installation exclude this error
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude Threat Detector Launcher Died With 70
    Register Cleanup    Exclude Threat Detector Launcher Died With 11
    Register Cleanup    Exclude Scan Now Terminated
    Register Cleanup    Exclude Failed To Scan Files
    Register Cleanup    Exclude Failed To Send Scan Request To STD
    Register Cleanup    Exclude Failed To Read Message From Scanning Server
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
    register cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude Threat Detector Launcher Died With 70
    Register Cleanup    Exclude Threat Detector Launcher Died With 11
    Register Cleanup    Exclude Scan Now Terminated
    Register Cleanup    Exclude Failed To Scan Files
    Register Cleanup    Exclude Failed To Send Scan Request To STD
    Register Cleanup    Exclude Failed To Read Message From Scanning Server
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

AV Runs Scan With SXL Lookup Enable
    Mark Susi Debug Log
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   stderr=STDOUT
    Configure and check scan now with offset
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True
    Wait Until AV Plugin Log Contains With Offset   Sending threat detection notification to central   timeout=60
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now
    SUSI Debug Log Contains With Offset  Post-scan lookup succeeded


AV Runs Scan With SXL Lookup Disabled
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Mark AV Log
    Mark Susi Debug Log
    Mark Sophos Threat Detector Log

    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh   stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/three_hundred_eicars/  recursive=True

    Configure and check scan now with lookups disabled

    Wait Until AV Plugin Log Contains With Offset  Sending threat detection notification to central   timeout=60
    SUSI Debug Log Does Not Contain With Offset   Post-scan lookup succeeded
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now
    AV Plugin Log Does Not Contain   Failed to send shutdown request: Failed to connect to unix socket


AV Plugin restarts threat detector on customer id change
    #    Get ALC Policy doesn't have Core and Base
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Configuration Data Invalid
    Register Cleanup    Exclude Invalid Settings No Primary Product
    ${pid} =   Record Sophos Threat Detector PID

    ${id1} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id1}  userpassword=${id1}  username=${id1}
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}

    # Need to get restart after new policy sent
    Mark AV Log
    Mark Sophos Threat Detector Log

    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed
    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket   timeout=120
    Wait For Sophos Threat Detector to restart  ${pid}

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
    ...   Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed   timeout=5
    Check Sophos Threat Detector has same PID   ${pid}

    # change credentials, threat_detector should restart
    ${pid} =   Record Sophos Threat Detector PID

    ${id3} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id3}  userpassword=${id3}  username=${id3}
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}

    Mark AV Log
    Mark Sophos Threat Detector Log
    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed
    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket   timeout=120
    Wait For Sophos Threat Detector to restart   ${pid}

AV Plugin restarts threat detector on susi startup settings change
    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=true
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed   timeout=60
    AV Plugin Log Does Not Contain With Offset  Failed to send shutdown request: Failed to connect to unix socket

    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be disabled   timeout=180
    Check Sophos Threat Detector has different PID   ${pid}

    # don't change lookup setting, threat_detector should not restart
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id2} =   Generate Random String
    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Run Keyword And Expect Error
    ...   Keyword 'AV Plugin Log Contains With Offset' failed after retrying for 5 seconds.*
    ...   Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed   timeout=5
    Check Sophos Threat Detector has same PID   ${pid}

    # change lookup setting, threat_detector should restart
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id3} =   Generate Random String
    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=true
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed   timeout=60
    AV Plugin Log Does Not Contain With Offset  Failed to send shutdown request: Failed to connect to unix socket
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Check Sophos Threat Detector has different PID   ${pid}

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be enabled   timeout=180

AV Plugin tries to restart threat detector on susi startup settings change
    Register Cleanup    Exclude Threat Detector Launcher Died
    Register Cleanup    Exclude Failed To Write To UnixSocket Environment Interuption
    Register Cleanup    Exclude Invalid Settings No Primary Product
    Register Cleanup    Exclude Configuration Data Invalid

    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=true
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Mark AV Log
    Mark Sophos Threat Detector Log
    stop sophos_threat_detector

    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed   timeout=60
    AV Plugin Log Contains With Offset  Failed to connect to Sophos Threat Detector Controller - retrying after sleep
    Wait Until AV Plugin Log Contains With Offset  Reached total maximum number of connection attempts.

    start sophos_threat_detector
    Wait until threat detector running with offset

    # change lookup setting, threat_detector should restart
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id3} =   Generate Random String
    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=true
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system/susi configuration has changed   timeout=60
    AV Plugin Log Does Not Contain With Offset  Failed to send shutdown request: Failed to connect to unix socket
    AV Plugin Log Does Not Contain With Offset  Failed to connect to Sophos Threat Detector Controller - retrying after sleep
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=120
    Check Sophos Threat Detector has different PID   ${pid}

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be enabled   timeout=180

Sophos Threat Detector sets default if susi startup settings permissions incorrect
    [Tags]  FAULT INJECTION
    Register Cleanup    Exclude Configuration Data Invalid
    Register Cleanup    Exclude Invalid Settings No Primary Product
    Restart sophos_threat_detector
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Mark AV Log
    Mark Sophos Threat Detector Log

    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=120

    Run Process  chmod  000  ${SUSI_STARTUP_SETTINGS_FILE}
    Run Process  chmod  000  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    Mark Sophos Threat Detector Log
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -9   ${output}

    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket
    ...   timeout=120

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset   Turning Live Protection on as default - no susi startup settings found

AV Plugin Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup  Empty Directory  /opt/sophos-spl/base/mcs/event
    Empty Directory  /opt/sophos-spl/base/mcs/event/

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

    Empty Directory  /opt/sophos-spl/base/mcs/event/

    Run  chmod 444 ${AV_LOG_PATH}
    Register Cleanup  Stop AV Plugin
    Register Cleanup  Run  chmod 600 ${AV_LOG_PATH}

    ${INITIAL_AV_PID} =  Record AV Plugin PID
    Log  Initial PID: ${INITIAL_AV_PID}
    Stop AV Plugin
    Start AV Plugin
    ${END_AV_PID} =  Record AV Plugin PID
    Log  Restarted PID: ${END_AV_PID}
    # Verify the restart actually happened
    Should Not Be Equal As Integers  ${INITIAL_AV_PID}  ${END_AV_PID}

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
    Register Cleanup    Exclude Permission Denied Setting Default Values For Susi Startup Settings
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
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml

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
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml

    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=250
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=18
    Log File  ${CLOUDSCAN_LOG_PATH}
    File Log Should Not Contain With Offset  ${CLOUDSCAN_LOG_PATH}  Detected "${NORMAL_DIRECTORY}/naughty_eicar" is infected with EICAR-AV-Test  ${LOG_MARK}
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '/tmp_test/naughty_eicar'"

Sophos Threat Detector always writes susi startup settings following a restart
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Restart AV Plugin And Clear The Logs For Integration Tests
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60

    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=true
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempSavPolicy.xml  ${policyContent}
    Mark AV Log
    Mark Sophos Threat Detector Log
    Send Sav Policy To Base  tempSavPolicy.xml

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    AV Plugin Log Contains  SAV policy received for the first time.
    Run Keyword And Ignore Error  Wait Until AV Plugin Log Contains  Restarting sophos_threat_detector as the system/susi configuration has changed
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=180
    Remove File  ${SUSI_STARTUP_SETTINGS_FILE}
    Remove File  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    Restart AV Plugin And Clear The Logs For Integration Tests

    Wait Until File exists  ${SUSI_STARTUP_SETTINGS_FILE}
    Wait Until File exists  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}
    Threat Detector Does Not Log Contain  Turning Live Protection on as default - no susi startup settings found

