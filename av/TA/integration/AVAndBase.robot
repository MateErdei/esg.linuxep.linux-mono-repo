*** Settings ***
Documentation    Integration tests for AVP and Base
Force Tags      INTEGRATION  AVBASE  TAP_PARALLEL1
Library         Collections
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ${COMMON_TEST_LIBS}/OnFail.py
Library         ${COMMON_TEST_LIBS}/OSUtils.py
Library         ../Libs/ThreatReportUtils.py
Library         ../Libs/Telemetry.py

Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/SafeStoreResources.robot

Suite Setup     AVAndBase Suite Setup
Suite Teardown  AVAndBase Suite Teardown

Test Setup      AVAndBase Test Setup

Test Teardown   AV And Base Teardown

Default Tags    TAP_TESTS

*** Keywords ***
AVAndBase Suite Setup
    Install With Base SDDS

AVAndBase Suite Teardown
    Uninstall All

AVAndBase Test Setup
    AV And Base Setup
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/disable_safestore

Remove Users Stop Processes
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep av
    Run Process   /bin/kill   -SIGKILL   ${output}
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -SIGKILL   ${output}
    Run Process  /usr/sbin/userdel   sophos-spl-av
    Run Process  /usr/sbin/userdel   sophos-spl-threat-detector

Wait Until Base Has Naughty Eicar Detection Event
    Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${SCAN_DIRECTORY}/naughty_eicar

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
    Configure and run scan now

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
    ${cls_mark} =  Mark Log Size  ${LOG_FILE}
    Wait For Log Contains After Mark   ${LOG_FILE}   Scanning   ${cls_mark}  timeout=60

    # Start Scan Now
    Configure scan now
    ${av_mark} =  Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait For AV Log Contains After Mark    Starting scan Scan Now  ${av_mark}   timeout=5

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    ${cls_mark} =  Mark Log Size  ${LOG_FILE}
    Wait For Log Contains After Mark   ${LOG_FILE}   Scanning   ${cls_mark}  timeout=30

    # Wait for Scan Now to complete
    Wait For AV Log Contains After Mark    Completed scan  ${av_mark}  timeout=180
    Wait For AV Log Contains After Mark    Sending scan complete  ${av_mark}

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    ${cls_mark2} =  Mark Log Size  ${LOG_FILE}
    Wait For Log Contains After Mark   ${LOG_FILE}   Scanning   ${cls_mark2}  timeout=30

    # Stop CLS
    ${result} =   Terminate Process  ${cls_handle}
    deregister cleanup  Terminate Process  ${cls_handle}

    Should Contain  ${{ [ ${EXECUTION_INTERRUPTED}, ${SCAN_ABORTED_WITH_THREAT} ] }}   ${result.rc}

AV plugin runs CLS while scan now is running

    # create something for scan now to work on
    Create Big Dir   count=60   path=/tmp_test/bigdir/

    # start scan now
    Configure scan now
    ${av_mark} =  Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait For AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}  timeout=5

    # ensure avscanner is working
    check avscanner can detect eicar

    # check ScanNow is still scanning
    Check AV Log Does Not Contain After Mark   Completed scan  ${av_mark}

    # wait for scan now to complete
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180
    Wait For AV Log Contains After Mark  Sending scan complete  ${av_mark}

AV plugin runs scan now twice consecutively
    Configure and run scan now
    ${av_mark} =  Mark AV Log
    Run Scan Now After Mark  ${av_mark}

AV plugin attempts to run scan now twice simultaneously
    Register On Fail  dump log  ${SCANNOW_LOG_PATH}
    ${av_mark} =  Mark AV Log
    Run Process  bash  ${BASH_SCRIPTS_PATH}/fileMaker.sh  2500  stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/file_maker/  recursive=True
    Configure scan now

    Send Sav Action To Base  ScanNow_Action.xml
    Wait For AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}  timeout=5

    Send Sav Action To Base  ScanNow_Action.xml

    ## Wait for 1 scan to happen
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180
    Wait For AV Log Contains After Mark  Sending scan complete  ${av_mark}

    ## Check we refused to start the second scan
    Check AV Log Contains After Mark  Refusing to run a second Scan named: Scan Now  ${av_mark}

    ## Check we started only one scan
    Check Log Contains N Times After Mark   ${AV_LOG_PATH}  Starting scan Scan Now  ${1}  ${av_mark}

AV plugin runs scheduled scan
    ${av_mark} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark}  timeout=150
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180


AV plugin can detect PUAs via scheduled scan
    Create File  /tmp_test/eicar_pua.com   ${EICAR_PUA_STRING}
    Register Cleanup  Remove Files  /tmp_test/eicar_pua.com

    ${av_mark} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark}  timeout=150
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180
    Wait For AV Log Contains After Mark  Found 'EICAR-PUA-Test'  ${av_mark}

    ${av_mark2} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base PUA Detections Disabled
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated After Mark  ${av_mark2}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark2}  timeout=150
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark2}  timeout=180
    Check AV Log Does Not Contain After Mark   Found 'EICAR-PUA-Test'  ${av_mark2}


AV plugin runs multiple scheduled scans
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude File Name Too Long For Cloud Scan
    ${av_mark} =  Mark AV Log
    Register Cleanup  Restart AV Plugin And Clear The Logs For Integration Tests
    Send Sav Policy With Multiple Imminent Scheduled Scans To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark}  timeout=150
    Wait For AV Log Contains After Mark  Refusing to run a second Scan named: Sophos Cloud Scheduled Scan  ${av_mark}  timeout=120

AV plugin runs scheduled scan after restart
    Send Sav Policy With Imminent Scheduled Scan To Base
    Stop AV Plugin And Threat Detector
    ${av_mark} =  Mark AV Log
    Start AV Plugin And Threat Detector
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark}  timeout=150
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180
    Exclude Communication Between AV And Base Due To No Incoming Data

AV plugin reports an info message if no policy is received
    Stop AV Plugin And Threat Detector
    Remove File     ${MCS_PATH}/policy/ALC-1_policy.xml
    Remove File     ${MCS_PATH}/policy/SAV-2_policy.xml

    ${av_mark} =  Mark AV Log
    Start AV Plugin And Threat Detector
    Wait For AV Log Contains After Mark  Failed to request SAV policy at startup (No Policy Available)  ${av_mark}
    Wait For AV Log Contains After Mark  Failed to request ALC policy at startup (No Policy Available)  ${av_mark}

AV plugin fails scan now if no policy
    Register Cleanup    Exclude Scan As Invalid
    Stop AV Plugin And Threat Detector
    Remove File     ${MCS_PATH}/policy/SAV-2_policy.xml
    ${av_mark} =  Mark AV Log
    Start AV Plugin And Threat Detector

    Wait until AV Plugin running after mark  ${av_mark}

    ${av_mark} =  Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait For AV Log Contains After Mark  Evaluating Scan Now  ${av_mark}
    Check AV Log Contains After Mark  Refusing to run invalid scan: INVALID  ${av_mark}

AV plugin SAV Status contains revision ID of policy
    Register Cleanup    Exclude Invalid Day From Policy
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini
    ${revId} =    Set Variable    ac9eaa2f09914ce947cfb14f1326b802ef0b9a86eca7f6c77557564e36dbff9a
    Send Sav Policy With No Scheduled Scans    revId=${revId}
    Wait Until SAV Status XML Contains  Res="Same" RevID="${revId}"  timeout=60
    SAV Status XML Contains  <product-version>${version}</product-version>

AV plugin sends Scan Complete event and (fake) Report To Central
    ${now} =  Get Current Date  result_format=epoch
    ${av_mark} =  Mark AV Log
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_No_Scans.xml
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until Management Log Contains  Action SAV_action
    Wait For AV Log Contains After Mark  Starting scan  ${av_mark}
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180
    Wait For AV Log Contains After Mark  Sending scan complete  ${av_mark}
    Validate latest Event  ${now}

AV Gets SAV Policy When Plugin Restarts
    Send Sav Policy With No Scheduled Scans
    register cleanup  remove file   ${MCS_PATH}/policy/SAV-2_policy.xml
    register cleanup  remove file   ${SUSI_STARTUP_SETTINGS_FILE}
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Stop AV Plugin And Threat Detector
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}

    Start AV Plugin And Threat Detector
    Wait For Log Contains From Mark  ${av_mark}  SAV policy received for the first time.
    Wait For Log Contains From Mark  ${av_mark}  Processing SAV policy
    Wait Until Created  ${SUSI_STARTUP_SETTINGS_FILE}
    Wait Until Created  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}
    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For Log Contains From Mark  ${av_mark}  Configured number of Scheduled Scans: 0

AV Plugin Processes First SAV Policy Correctly After Initial Wait For Policy Fails
    Stop AV Plugin And Threat Detector
    Remove File    ${SUSI_STARTUP_SETTINGS_FILE}
    Remove File    ${MCS_PATH}/policy/SAV-2_policy.xml

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}

    Start AV Plugin And Threat Detector
    Wait For Log Contains From Mark  ${av_mark}  SAV policy has not been sent to the plugin
    Send Sav Policy With No Scheduled Scans
    Wait For Log Contains From Mark  ${av_mark}  Processing SAV policy
    Wait Until Created   ${SUSI_STARTUP_SETTINGS_FILE}
    Wait Until Created   ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

AV Gets ALC Policy When Plugin Restarts
    Register Cleanup    Exclude UpdateScheduler Fails

    Send Alc Policy
    File Should Exist  ${MCS_PATH}/policy/ALC-1_policy.xml
    Stop AV Plugin And Threat Detector

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}

    Start AV Plugin And Threat Detector
    Wait until AV Plugin running after mark  ${av_mark}
    Wait until threat detector running after mark  ${td_mark}

    Wait For Log Contains From Mark  ${av_mark}  Received policy from management agent for AppId: ALC
    Wait For Log Contains From Mark  ${av_mark}  ALC policy received for the first time.
    Wait For Log Contains From Mark  ${av_mark}  Processing ALC policy

    check_sophos_threat_detector_log_does_not_contain_after_mark  Failed to read customerID - using default value  mark=${td_mark}

AV Configures No Scheduled Scan Correctly
    Register Cleanup    Exclude UpdateScheduler Fails
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    ${av_mark} =  Mark AV Log
    Send Sav Policy With No Scheduled Scans
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated
    Wait For AV Log Contains After Mark  Configured number of Scheduled Scans: 0  ${av_mark}
    Check AV Log Does Not Contain After Mark  Unable to accept policy as scan information is invalid. Following scans wont be run:  ${av_mark}


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
    ${cls_mark} =  Mark Log Size  ${LOG_FILE}
    Wait For Log Contains After Mark   ${LOG_FILE}   Scanning   ${cls_mark}  timeout=${60}

    ${av_mark} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    Wait For AV Log Contains After Mark  Configured number of Scheduled Scans: 1  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark}  timeout=${150}

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    ${cls_mark} =  Mark Log Size  ${LOG_FILE}
    Wait For Log Contains After Mark   ${LOG_FILE}   Scanning   ${cls_mark}  timeout=${60}

    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=${180}

    # check CLS is still scanning
    Process Should Be Running   ${cls_handle}
    ${cls_mark2} =  Mark Log Size  ${LOG_FILE}
    Wait For Log Contains After Mark   ${LOG_FILE}   Scanning   ${cls_mark2}  timeout=${60}

    # Stop CLS
    ${result} =   Terminate Process  ${cls_handle}
    Should Contain  ${{ [ ${EXECUTION_INTERRUPTED}, ${SCAN_ABORTED_WITH_THREAT} ] }}   ${result.rc}

AV plugin runs CLS while scheduled scan is running
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    ${av_mark} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base

    # create something for scheduled scan to work on
    Create Big Dir   count=60   path=/tmp_test/bigdir/

    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark}  timeout=150

    check avscanner can detect eicar

    Check AV Log Does Not Contain After Mark   Completed scan  ${av_mark}
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180

AV Configures Single Scheduled Scan Correctly
    ${av_mark} =  Mark AV Log
    Send Fixed Sav Policy
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Scheduled Scans: 1  ${av_mark}
    Wait For AV Log Contains After Mark  Scheduled Scan: Sophos Cloud Scheduled Scan  ${av_mark}
    Wait For AV Log Contains After Mark  Days: Monday  ${av_mark}
    Wait For AV Log Contains After Mark  Times: 11:00:00  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Exclusions: 28  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Sophos Defined Extension Exclusions: 3  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of User Defined Extension Exclusions: 4  ${av_mark}

AV Configures Multiple Scheduled Scans Correctly
    ${av_mark} =  Mark AV Log
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Scheduled Scans: 2  ${av_mark}
    Wait For AV Log Contains After Mark  Scheduled Scan: Sophos Cloud Scheduled Scan One  ${av_mark}
    Wait For AV Log Contains After Mark  Days: Tuesday Saturday  ${av_mark}
    Wait For AV Log Contains After Mark  Times: 04:00:00 16:00:00  ${av_mark}
    Wait For AV Log Contains After Mark  Scheduled Scan: Sophos Cloud Scheduled Scan Two  ${av_mark}
    Wait For AV Log Contains After Mark  Days: Monday Thursday  ${av_mark}
    Wait For AV Log Contains After Mark  Times: 11:00:00 23:00:00  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Exclusions: 25  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Sophos Defined Extension Exclusions: 0  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of User Defined Extension Exclusions: 0  ${av_mark}

AV Reconfigures Scans Correctly
    ${av_mark} =  Mark AV Log
    Send Fixed Sav Policy
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark  ${av_mark}
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    Wait For AV Log Contains After Mark  Scheduled Scan: Sophos Cloud Scheduled Scan  ${av_mark}
    Wait For AV Log Contains After Mark  Days: Monday  ${av_mark}
    Wait For AV Log Contains After Mark  Times: 11:00:00  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Exclusions: 28  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Sophos Defined Extension Exclusions: 3  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of User Defined Extension Exclusions: 4  ${av_mark}
    Send Sav Policy With Multiple Scheduled Scans
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Scheduled Scans: 2  ${av_mark}
    Wait For AV Log Contains After Mark  Scheduled Scan: Sophos Cloud Scheduled Scan One  ${av_mark}
    Wait For AV Log Contains After Mark  Days: Tuesday Saturday  ${av_mark}
    Wait For AV Log Contains After Mark  Times: 04:00:00 16:00:00  ${av_mark}
    Wait For AV Log Contains After Mark  Scheduled Scan: Sophos Cloud Scheduled Scan Two  ${av_mark}
    Wait For AV Log Contains After Mark  Days: Monday Thursday  ${av_mark}
    Wait For AV Log Contains After Mark  Times: 11:00:00 23:00:00  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Exclusions: 25  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of Sophos Defined Extension Exclusions: 0  ${av_mark}
    Wait For AV Log Contains After Mark  Configured number of User Defined Extension Exclusions: 0  ${av_mark}

AV Deletes Scan Correctly
    ${avmark} =  Mark AV Log
    Send Complete Sav Policy
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark   ${avmark}
    AV Plugin Log Contains  Configured number of Scheduled Scans: 1
    wait_for_av_log_contains_after_mark  Scheduled Scan: Sophos Cloud Scheduled Scan   ${avmark}
    wait_for_av_log_contains_after_mark  Days: Monday   ${avmark}
    wait_for_av_log_contains_after_mark  Times: 11:00:00   ${avmark}

    ${avmark} =  Mark AV Log
    Send Sav Policy With No Scheduled Scans
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml
    Wait until scheduled scan updated After Mark   ${avmark}
    wait_for_av_log_contains_after_mark  Configured number of Scheduled Scans: 0   ${avmark}

AV Plugin Reports Threat XML To Base
    Empty Directory  ${MCS_PATH}/event/
    Register Cleanup  Empty Directory  ${MCS_PATH}/event
    ${avmark} =  Mark AV Log

    Create File     ${SCAN_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output   /usr/local/bin/avscanner ${SCAN_DIRECTORY}/naughty_eicar

    Log   ${output}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${avmark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until Base Has Naughty Eicar Detection Event

Avscanner runs as non-root
    Empty Directory  ${MCS_PATH}/event/
    Register Cleanup  Empty Directory  ${MCS_PATH}/event/
    Register Cleanup  List Directory  ${MCS_PATH}/event/
    ${avmark} =  Mark AV Log

    Create File     ${SCAN_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${command} =    Set Variable    /usr/local/bin/avscanner ${SCAN_DIRECTORY}/naughty_eicar
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log   ${output}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/naughty_eicar" is infected with EICAR-AV-Test (On Demand)

    Should Not Contain    ${output}    Failed to read the config file
    Should Not Contain    ${output}    All settings will be set to their default value
    Should Contain   ${output}    Logger av configured for level: DEBUG

    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${avmark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until Base Has Naughty Eicar Detection Event

AV Plugin Reports encoded eicars To Base
   Register Cleanup  Exclude Failed To connect To Warehouse Error
   Create Encoded Eicars
   register cleanup  Remove Directory  /tmp_test/encoded_eicars  true

   Empty Directory  ${MCS_PATH}/event/
   Register Cleanup  Empty Directory  ${MCS_PATH}/event/
   Register Cleanup  List Directory  ${MCS_PATH}/event/

   ${expected_count} =  Count Eicars in Directory  /tmp_test/encoded_eicars/

   # We send both CORE Detection Events and CORE Clean Events, so for every detection there will be 2 events.
   ${expected_count}=    Evaluate    ${expected_count} * 2

   Should Be True  ${expected_count} > ${0}

   ${result} =  Run Process  /usr/local/bin/avscanner  /tmp_test/encoded_eicars/  timeout=120s  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  ${VIRUS_DETECTED_RESULT}
   Log  ${result.stdout}

   #make sure base has generated all events before checking
   #assumes no other MCS events
   Wait Until Keyword Succeeds
         ...  60 secs
         ...  5 secs
         ...  Base Has Number Of CORE Events  ${expected_count}

   check_all_eicars_are_found  /tmp_test/encoded_eicars/

AV Plugin uninstalls
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Install With Base SDDS
    Check avscanner in /usr/local/bin
    Run plugin uninstaller
    Check avscanner not in /usr/local/bin
    Check AV Plugin Not Installed

AV Plugin Saves Logs On Downgrade
    Register Cleanup  Exclude MCS Router is dead
    Register Cleanup  Install With Base SDDS
    Check AV Plugin Running
    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed
    Check Logs Saved On Downgrade

AV Plugin Saves SafeStore and Detections Databases On Downgrade
    Register Cleanup  Exclude MCS Router is dead
    Register Cleanup  Install With Base SDDS
    Check AV Plugin Running

    Wait Until AV Plugin Log Contains    Initialised Threat Database

    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed
    Wait Until File exists  ${SAFESTORE_BACKUP_DIR}/persist-threatDatabase
    Verify SafeStore Database Backups Exist in Path    ${SAFESTORE_BACKUP_DIR}

    Install AV Directly from SDDS
    File should exist    ${SOPHOS_INSTALL}/plugins/av/var/persist-threatDatabase
    ${user} =     get_file_owner    ${SOPHOS_INSTALL}/plugins/av/var/persist-threatDatabase
    ${group} =     get_file_group    ${SOPHOS_INSTALL}/plugins/av/var/persist-threatDatabase
    Should be equal as Strings  sophos-spl-av    ${user}
    Should be equal as Strings  sophos-spl-group    ${group}

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

    ${av_mark} =  Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait For AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}  timeout=5

    Move File  ${SOPHOS_THREAT_DETECTOR_BINARY}.0  ${SOPHOS_THREAT_DETECTOR_BINARY}_moved
    Register Cleanup    Restart AV Plugin And Clear The Logs For Integration Tests
    Register Cleanup    Move File  ${SOPHOS_THREAT_DETECTOR_BINARY}_moved  ${SOPHOS_THREAT_DETECTOR_BINARY}.0
    ${pid} =   Record Sophos Threat Detector PID
    Run Process   /bin/kill   -SIGSEGV   ${pid}

    Wait For AV Log Contains After Mark  Scan: Scan Now, terminated with exit code: ${SCAN_ABORTED}  ${av_mark}
    ...  timeout=${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD}

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

    ${av_mark} =  Mark AV Log
    Register Cleanup    Remove File  ${SCANNOW_LOG_PATH}
    Remove File  ${SCANNOW_LOG_PATH}
    Register On Fail  dump log  ${SCANNOW_LOG_PATH}

    Send Sav Action To Base  ScanNow_Action.xml
    Wait For AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}  timeout=5
    ## Scan only takes ~3 seconds once scanning starts, so we need to finish this quickly
    Wait For AV Log Contains After Mark  Sending threat detection notification to central  ${av_mark}  timeout=60

    Move File  ${SOPHOS_THREAT_DETECTOR_BINARY}.0  ${SOPHOS_THREAT_DETECTOR_BINARY}_moved
    Register Cleanup    Restart AV Plugin And Clear The Logs For Integration Tests
    Register Cleanup    Move File  ${SOPHOS_THREAT_DETECTOR_BINARY}_moved  ${SOPHOS_THREAT_DETECTOR_BINARY}.0
    ${pid} =   Record Sophos Threat Detector PID
    Run Process   /bin/kill   -SIGSEGV   ${pid}

    Wait For AV Log Contains After Mark  Scan: Scan Now, found threats but aborted with exit code: ${SCAN_ABORTED_WITH_THREAT}
    ...  ${av_mark}  timeout=${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD}

AV Plugin does not restart threat detector on customer id change
    #    Get ALC Policy doesn't have Core and Base
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Configuration Data Invalid
    Register Cleanup    Exclude Invalid Settings No Primary Product
    # Allow TD to settle after test setup
    Sleep  ${1}
    ${pid} =   Record Sophos Threat Detector PID

    # scan eicar to ensure susi is loaded, so that we know which log messages to expect later
    Check avscanner can detect eicar

    ${id1} =   Generate Random String
    ${uuid1} =   Set Variable  eeeeeeee-aaaa-ad6b-f90d-abcdefa54b7d
    ${uuid2} =   Set Variable  ffffffff-aaaa-ad6b-f90d-fedcbaa54b7d

    ${policyContent} =   Get ALC Policy   revid=${id1}  userpassword=${id1}  username=${id1}
    ...  customer_id=${uuid1}
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}

    ${av_mark} =  Mark AV Log
    ${threat_detector_mark} =  Mark Sophos Threat Detector Log

    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait For AV Log Contains After Mark   Received new policy  ${av_mark}
    Wait For AV Log Contains After Mark   Reloading susi as policy configuration has changed  ${av_mark}
    Wait For Sophos Threat Detector Log Contains After Mark   Susi configuration reloaded  ${threat_detector_mark}
    Check Sophos Threat Detector has same PID   ${pid}

    # change revid only, threat_detector should not restart
    ${pid} =   Record Sophos Threat Detector PID

    ${id2} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id2}  userpassword=${id1}  username=${id1}
    ...  customer_id=${uuid1}

    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}
    ${av_mark2} =  Mark AV Log
    ${threat_detector_mark2} =  Mark Sophos Threat Detector Log
    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait For AV Log Contains After Mark   Received new policy  ${av_mark2}
    Run Keyword And Expect Error
    ...   Failed to find b'Reloading susi as policy configuration has changed' in ${AV_LOG_PATH}
    ...   Wait For AV Log Contains After Mark   Reloading susi as policy configuration has changed  ${av_mark2}   timeout=5
    Check Sophos Threat Detector has same PID   ${pid}

    # change credentials, avp should issue not a susi reload request
    ${pid} =   Record Sophos Threat Detector PID

    ${id3} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id3}  userpassword=${id3}  username=${id3}
    ...  customer_id=${uuid2}
    Log   ${policyContent}
    Create File  ${RESOURCES_PATH}/tempAlcPolicy.xml  ${policyContent}

    ${av_mark3} =  Mark AV Log
    ${threat_detector_mark3} =  Mark Sophos Threat Detector Log
    Send Alc Policy To Base  tempAlcPolicy.xml

    Wait For AV Log Contains After Mark   Received new policy  ${av_mark3}
    Wait For AV Log Contains After Mark   Reloading susi as policy configuration has changed  ${av_mark3}
    Wait For Sophos Threat Detector Log Contains After Mark   Susi configuration reloaded  ${threat_detector_mark3}
    Check Sophos Threat Detector has same PID   ${pid}


AV Plugin tries to restart threat detector when SXL Lookup disabled
    Register Cleanup    Exclude Failed To Write To UnixSocket Environment Interuption
    Register Cleanup    Exclude Invalid Settings No Primary Product
    Register Cleanup    Exclude Configuration Data Invalid

    Comment  set our initial policy

    ${revid} =   Generate Random String
    ${policyContent} =   create_corc_policy  revid=${revid}  sxlLookupEnabled=${true}
    Log   ${policyContent}

    ${av_mark} =  Mark AV Log
    Send CORC Policy To Base From Content  ${policyContent}
    Wait For AV Log Contains After Mark   Received new policy  ${av_mark}

    stop sophos_threat_detector

    Comment  disable SXL lookups, AV should try to restart TD

    ${revid} =   Generate Random String
    ${policyContent} =   create_corc_policy  revid=${revid}  sxlLookupEnabled=${false}
    Log   ${policyContent}
    ${av_mark} =  Mark AV Log
    Send CORC Policy To Base From Content  ${policyContent}

    Wait For AV Log Contains After Mark   Received new policy  ${av_mark}
    Wait For AV Log Contains After Mark   Restarting sophos_threat_detector as the configuration has changed  ${av_mark}   timeout=60
    Check AV Log Contains After Mark  ProcessControlClient failed to connect to ${COMPONENT_ROOT_PATH}/chroot/var/process_control_socket - retrying  ${av_mark}
    Wait For AV Log Contains After Mark  ProcessControlClient reached the maximum number of connection attempts  ${av_mark}

    ${threat_detector_mark2} =  Mark Sophos Threat Detector Log
    start sophos_threat_detector
    Wait until threat detector running after mark  ${threat_detector_mark2}

    Comment  change lookup setting, threat_detector should restart

    ${av_mark2} =  Mark AV Log
    ${threat_detector_mark3} =  Mark Sophos Threat Detector Log

    ${revid} =   Generate Random String
    ${policyContent} =   create_corc_policy  revid=${revid}  sxlLookupEnabled=${true}
    Log   ${policyContent}
    Send CORC Policy To Base From Content  ${policyContent}

    Wait For AV Log Contains After Mark   Received new policy  ${av_mark2}
    Wait For AV Log Contains After Mark   Restarting sophos_threat_detector as the configuration has changed  ${av_mark2}   timeout=60
    Check AV Log Does Not Contain After Mark  Failed to send shutdown request: Failed to connect to unix socket  ${av_mark2}
    Check AV Log Does Not Contain After Mark  Failed to connect to ${COMPONENT_ROOT_PATH}/chroot/var/process_control_socket - retrying after sleep  ${av_mark2}

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar

    Wait For Sophos Threat Detector Log Contains After Mark  SXL Lookups will be enabled   ${threat_detector_mark3}  timeout=180

AV Plugin Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Empty Directory  ${MCS_PATH}/event/
    Register Cleanup    List Directory  ${MCS_PATH}/event/
    Empty Directory    ${MCS_PATH}/event/

    clear outbreak mode if required

    ${avmark} =  Mark AV Log

    Create File  ${NORMAL_DIRECTORY}/naughty_eicar  ${EICAR_STRING}
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/naughty_eicar

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar

    # Verify that we get a log message for the eicar
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${avmark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar

    # Verify that the AV Plugin sends an alert when logging is working
    Wait Until Base Has Naughty Eicar Detection Event

    Empty Directory  ${MCS_PATH}/event/
    #Reset Database otherwise our second base detection wont take place
    Reset ThreatDatabase

    Run  chmod 444 ${AV_LOG_PATH}
    Register Cleanup  Stop AV Plugin And Threat Detector
    Register Cleanup  Run  chmod 600 ${AV_LOG_PATH}

    ${INITIAL_AV_PID} =  Record AV Plugin PID
    Log  Initial PID: ${INITIAL_AV_PID}
    Stop AV Plugin And Threat Detector
    # Ensure threat reporting socket is deleted before restarting AV Plugin
    Remove File  ${COMPONENT_ROOT_PATH}/chroot/var/threat_report_socket
    Start AV Plugin And Threat Detector
    ${END_AV_PID} =  Record AV Plugin PID
    Log  Restarted PID: ${END_AV_PID}
    # Verify the restart actually happened
    Should Not Be Equal As Integers  ${INITIAL_AV_PID}  ${END_AV_PID}

    # Can't verify reporting socket is ready from logs so just have to wait:
    wait until created  ${COMPONENT_ROOT_PATH}/chroot/var/threat_report_socket
    Sleep  5s  wait for av process to start up

    ${result} =  Run Process  ls  -l  ${AV_LOG_PATH}
    Log  New permissions: ${result.stdout}

    ${avmark} =  Mark AV Log
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar

    # Verify the av plugin still sent the alert
    Wait Until Base Has Naughty Eicar Detection Event

    # Verify that the log wasn't written (permission blocked)
    AV Plugin Log Should Not Contain Detection Name And Path After Mark  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar   ${avmark}

Scan Now Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    Register Cleanup    Exclude SPL Base Not In Subscription Of The Policy
    Register Cleanup    Exclude Core Not In Policy Features
    Register Cleanup    Remove File  ${SCANNOW_LOG_PATH}
    Remove File  ${SCANNOW_LOG_PATH}

    ${avmark} =  Mark AV Log
    ${file_name} =       Set Variable   ScanNowWorksDespiteReadOnly

    Create File  /tmp_test/${file_name}  ${EICAR_STRING}
    Register Cleanup    Remove File  /tmp_test/${file_name}

    Configure scan now

    Send Sav Action To Base  ScanNow_Action.xml

    wait_for_av_log_contains_after_mark  Starting scan Scan Now  ${avmark}  timeout=40
    wait_for_av_log_contains_after_mark  Completed scan  ${avmark}  timeout=180
    wait_for_av_log_contains_after_mark  Sending scan complete   ${avmark}
    Log File  ${SCANNOW_LOG_PATH}
    File Log Contains  ${SCANNOW_LOG_PATH}  Detected "/tmp_test/${file_name}" is infected with EICAR-AV-Test (Scheduled)
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${avmark}  EICAR-AV-Test  /tmp_test/${file_name}

    ${result} =  Run Process  ls  -l  ${SCANNOW_LOG_PATH}
    Log  Old permissions: ${result.stdout}

    Run  chmod 444 '${SCANNOW_LOG_PATH}'
    Register Cleanup  Run  chmod 600 '${SCANNOW_LOG_PATH}'

    ${result} =  Run Process  ls  -l  ${SCANNOW_LOG_PATH}
    Log  New permissions: ${result.stdout}

    Configure scan now
    ${avmark} =  Mark AV Log
    ${scan_now_mark} =  Mark Scan Now Log

    Send Sav Action To Base  ScanNow_Action.xml

    wait_for_av_log_contains_after_mark  Starting scan Scan Now  ${avmark}  timeout=40
    wait_for_av_log_contains_after_mark  Completed scan  ${avmark}  timeout=180
    wait_for_av_log_contains_after_mark  Sending scan complete  ${avmark}
    Log File  ${SCANNOW_LOG_PATH}
    Check Scan Now Log Does Not Contain After Mark  Detected "${NORMAL_DIRECTORY}/${file_name}" is infected with EICAR-AV-Test  ${scan_now_mark}
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${avmark}  EICAR-AV-Test  /tmp_test/${file_name}


First SAV Policy With Invalid Day And Time Is Not Accepted
    Remove File   ${MCS_PATH}/policy/SAV-2_policy.xml
    File Should Not Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    ${av_mark} =  Mark AV Log
    Restart AV Plugin And Threat Detector
    Wait For AV Log Contains After Mark  SAV policy has not been sent to the plugin  ${av_mark}

    ${av_mark} =  Mark AV Log

    Send Invalid Sav Policy
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait For AV Log Contains After Mark  Invalid day from policy: TheDayOfDays  ${av_mark}
    AV Log Contains Multiple Times After Mark   Invalid day from policy:  ${av_mark}  times=2
    Wait For AV Log Contains After Mark  Invalid time from policy: 45:67:89  ${av_mark}
    Wait For AV Log Contains After Mark  Invalid time from policy: whatisthismadness  ${av_mark}
    AV Log Contains Multiple Times After Mark  Invalid time from policy:  ${av_mark}  times=3
    Wait For AV Log Contains After Mark  Unable to accept policy as scan information is invalid. Following scans wont be run:  ${av_mark}

    Check AV Log Does Not Contain After Mark  SAV policy received for the first time.  ${av_mark}
    Check AV Log Does Not Contain After Mark  Configured number of Scheduled Scans: 1  ${av_mark}
    Check AV Log Does Not Contain After Mark  Processing request to restart sophos threat detector  ${av_mark}


Scheduled Scan Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    Register Cleanup    Remove File  ${CLOUDSCAN_LOG_PATH}
    Register Cleanup    Exclude Failed To Write To UnixSocket Environment Interuption

    Remove File  ${CLOUDSCAN_LOG_PATH}

    ${file_name} =       Set Variable   ScheduledScanWorksDespiteReadOnly

    Create File  /tmp_test/${file_name}  ${EICAR_STRING}
    Register Cleanup    Remove File  /tmp_test/${file_name}

    ${avmark} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated After Mark   ${avmark}
    wait_for_av_log_contains_after_mark  Starting scan Sophos Cloud Scheduled Scan  ${avmark}  timeout=250
    wait_for_av_log_contains_after_mark  Completed scan  ${avmark}  timeout=18
    Log File  ${CLOUDSCAN_LOG_PATH}
    File Log Contains  ${CLOUDSCAN_LOG_PATH}  Detected "/tmp_test/${file_name}" is infected with EICAR-AV-Test (Scheduled)

    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${avmark}  EICAR-AV-Test  /tmp_test/${file_name}

    #Reset Database otherwise our second base detection wont take place
    Reset ThreatDatabase
    ${result} =  Run Process  ls  -l  ${CLOUDSCAN_LOG_PATH}
    Log  Old permissions: ${result.stdout}

    ${scheduledscanmark} =   Mark Scheduled Scan Log

    Run  chmod 444 '${CLOUDSCAN_LOG_PATH}'
    Register Cleanup  Run  chmod 600 '${CLOUDSCAN_LOG_PATH}'

    ${result} =  Run Process  ls  -l  ${CLOUDSCAN_LOG_PATH}
    Log  New permissions: ${result.stdout}

    ${avmark} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait until scheduled scan updated After Mark   ${avmark}
    wait_for_av_log_contains_after_mark  Starting scan Sophos Cloud Scheduled Scan  ${avmark}  timeout=250
    wait_for_av_log_contains_after_mark  Completed scan  ${avmark}  timeout=18
    Log File  ${CLOUDSCAN_LOG_PATH}
    check_log_does_not_contain_after_mark  ${CLOUDSCAN_LOG_PATH}  Detected "${NORMAL_DIRECTORY}/${file_name}" is infected with EICAR-AV-Test  ${scheduledscanmark}
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${avmark}  EICAR-AV-Test  /tmp_test/${file_name}


AV Does Not Fall Over When OA Config Is Read Only
    [Tags]  FAULT INJECTION

    ${avmark} =  Mark AV Log

    Wait Until Created   ${AV_VAR_DIR}/on_access_policy.json  timeout=10sec

    Run  chmod 444 '${AV_VAR_DIR}/on_access_policy.json'

    ${result} =  Run Process  ls  -l  ${AV_VAR_DIR}/on_access_policy.json
    Log  New permissions: ${result.stdout}

    Send Policies to enable on-access


AV Plugin Doesnt Read Response Action
    [Tags]  FAULT INJECTION
    ${av_mark} =  Mark AV Log
    Send RA Action To Base

    Wait For AV Log Contains After Mark  Ignoring action not in XML format   ${av_mark}
    check_log_does_not_contain_after_mark  ${AV_LOG_PATH}   av <> Exception encountered while parsing Action XML:  ${av_mark}


AV Runs Scan With SXL Lookup Enabled
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh  /tmp_test/eicars  10  stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/eicars/  recursive=True

    ${av_mark} =  Mark AV Log
    ${susi_debug_mark} =  Mark SUSI Debug Log
    Configure and run scan now
    Wait For AV Log Contains After Mark  Sending threat detection notification to central  ${av_mark}   timeout=60
    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}

    Check SUSI Debug Log Contains After Mark  Post-scan lookup succeeded  ${susi_debug_mark}


AV Runs Scan With SXL Lookup Disabled
    Run Process  bash  ${BASH_SCRIPTS_PATH}/eicarMaker.sh  /tmp_test/eicars  3  stderr=STDOUT
    Register Cleanup    Remove Directory    /tmp_test/eicars/  recursive=True

    ${av_mark} =  Mark AV Log
    ${susi_debug_mark} =  Mark SUSI Debug Log

    Configure and check scan now with lookups disabled

    Wait For AV Log Contains After Mark  Sending threat detection notification to central  ${av_mark}   timeout=60
    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}

    Check SUSI Debug Log Does Not Contain After Mark   Post-scan lookup started  ${susi_debug_mark}
    Check SUSI Debug Log Does Not Contain After Mark   Post-scan lookup succeeded  ${susi_debug_mark}
    AV Plugin Log Does Not Contain   Failed to send shutdown request: Failed to connect to unix socket

