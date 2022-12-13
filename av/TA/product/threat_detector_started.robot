*** Settings ***
Documentation   Product tests for sophos_threat_detector
Force Tags      PRODUCT  THREAT_DETECTOR

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/LockFile.py
Library         ../Libs/OnFail.py
Library         ../Libs/LogUtils.py

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Suite Setup    Threat Detector Suite Setup

Test Setup     Threat Detector Test Setup
Test Teardown  Threat Detector Test Teardown

*** Variables ***
${AV_PLUGIN_HANDLE}
${TESTSYSFILE}  hosts
${TESTSYSPATHBACKUP}  /etc/${TESTSYSFILE}backup
${TESTSYSPATH}  /etc/${TESTSYSFILE}
${SOMETIMES_SYMLINKED_SYSFILE}  resolv.conf
${SOMETIMES_SYMLINKED_SYSPATHBACKUP}  /etc/${SOMETIMES_SYMLINKED_SYSFILE}backup
${SOMETIMES_SYMLINKED_SYSPATH}  /etc/${SOMETIMES_SYMLINKED_SYSFILE}


*** Keywords ***

Threat Detector Suite Setup
    Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

Threat Detector Test Setup
    Component Test Setup
    Mark Sophos Threat Detector Log
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Register On Fail  Dump Log  ${AV_LOG_PATH}
    Start AV
    Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector
    Wait_For_Log_contains_after_last_restart  ${AV_LOG_PATH}  ScanProcessMonitor <> Config Monitor entering main loop

Threat Detector Test Teardown
    List AV Plugin Path
    run teardown functions
    Stop AV

    Exclude CustomerID Failed To Read Error
    Exclude Expected Sweep Errors
    Check All Product Logs Do Not Contain Error
    Component Test TearDown

Start AV
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    Mark AV Log
    Mark Sophos Threat Detector Log
    Check AV Plugin Not Running
    Check Threat Detector Not Running
    Check Threat Detector PID File Does Not Exist

    Start Sophos Threat Detector Under Fake Watchdog

    ${fake_management_log_path} =   FakeManagementLog.get_fake_management_log_path
    ${fake_management_log_mark} =  LogUtils.mark_log_size  ${fake_management_log_path}
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Register Cleanup   Terminate And Wait until AV Plugin not running  ${AV_PLUGIN_HANDLE}
    Check AV Plugin Installed from Marks  ${fake_management_log_mark}

Stop AV
    Stop Sophos Threat Detector Under Fake Watchdog
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}

Verify threat detector log rotated
    List Directory  ${AV_PLUGIN_PATH}/log/sophos_threat_detector
    Should Exist  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/sophos_threat_detector.log.1

Dump and Reset Logs
    Register Cleanup   Empty Directory   ${AV_PLUGIN_PATH}/log/sophos_threat_detector/
    Register Cleanup   Dump log          ${AV_PLUGIN_PATH}/log/sophos_threat_detector/sophos_threat_detector.log

Revert System File To Original
    copy file with permissions  ${TESTSYSPATHBACKUP}  ${TESTSYSPATH}
    Remove File  ${TESTSYSPATHBACKUP}

Revert Sometimes-symlinked System File To Original
    copy file with permissions  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}  ${SOMETIMES_SYMLINKED_SYSPATH}
    Remove File  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}
    
*** Test Cases ***

Threat Detector Restarts If System File Contents Change

    copy file with permissions  ${TESTSYSPATH}  ${TESTSYSPATHBACKUP}
    Register On Fail   Revert System File To Original

    ${av_mark} =  LogUtils.Get Av Log Mark
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark

    ${ORG_CONTENTS} =  Get File  ${TESTSYSPATH}  encoding_errors=replace
    Append To File  ${TESTSYSPATH}   "#NewLine"

    wait_for_av_log_contains_after_mark  System configuration updated for ${TESTSYSFILE}  ${av_mark}
    check_av_log_does_not_contain_after_mark  System configuration not changed for ${TESTSYSFILE}  ${av_mark}

    Wait until threat detector running after mark  ${td_mark}

    ${av_mark} =  LogUtils.Get Av Log Mark

    Revert System File To Original
    Deregister On Fail   Revert System File To Original

    wait_for_av_log_contains_after_mark  System configuration updated for ${TESTSYSFILE}  ${av_mark}
    check_av_log_does_not_contain_after_mark  System configuration not changed for ${TESTSYSFILE}  ${av_mark}

    ${POSTTESTCONTENTS} =  Get File  ${TESTSYSPATH}  encoding_errors=replace
    Should Be Equal   ${POSTTESTCONTENTS}   ${ORG_CONTENTS}


Threat Detector Does Not Restart If System File Contents Do Not Change
    ${av_mark} =  LogUtils.Get Av Log Mark

    copy file with permissions  ${TESTSYSPATH}  ${TESTSYSPATHBACKUP}
    Revert System File To Original

    ## LINUXDAR-5249 - we now just check all files every time
    wait_for_av_log_contains_after_mark  System configuration not changed  ${av_mark}
    check_av_log_does_not_contain_after_mark  System configuration updated for ${TESTSYSFILE}  ${av_mark}


Threat Detector Restarts If Sometimes-symlinked System File Contents Change
    ${av_mark} =  LogUtils.Get Av Log Mark
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark

    copy file  ${SOMETIMES_SYMLINKED_SYSPATH}  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}
    Register On Fail   Revert Sometimes-symlinked System File To Original

    ${ORG_CONTENTS} =  Get File  ${SOMETIMES_SYMLINKED_SYSPATH}  encoding_errors=replace
    Append To File  ${SOMETIMES_SYMLINKED_SYSPATH}   "#NewLine"

    wait_for_av_log_contains_after_mark  System configuration updated for ${SOMETIMES_SYMLINKED_SYSFILE}  ${av_mark}
    Wait until threat detector running after mark  ${td_mark}

    ${av_mark} =  LogUtils.Get Av Log Mark
    Revert Sometimes-symlinked System File To Original
    Deregister On Fail   Revert Sometimes-symlinked System File To Original

    wait_for_av_log_contains_after_mark  System configuration updated for ${SOMETIMES_SYMLINKED_SYSFILE}  ${av_mark}

    ${POSTTESTCONTENTS} =  Get File  ${SOMETIMES_SYMLINKED_SYSPATH}  encoding_errors=replace
    Should Be Equal   ${POSTTESTCONTENTS}   ${ORG_CONTENTS}


Threat Detector Does Not Restart If Sometimes-symlinked System File Contents Do Not Change
    ${av_mark} =  LogUtils.Get Av Log Mark

    copy file  ${SOMETIMES_SYMLINKED_SYSPATH}  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}
    Revert Sometimes-symlinked System File To Original

    wait_for_av_log_contains_after_mark  System configuration not changed  ${av_mark}
    check_av_log_does_not_contain_after_mark  System configuration updated for ${SOMETIMES_SYMLINKED_SYSFILE}  ${av_mark}


Threat Detector Restarts If System File Contents Change While A Long Scan Is Ongoing

    copy file with permissions  ${TESTSYSPATH}  ${TESTSYSPATHBACKUP}
    Register On Fail   Revert System File To Original

    ${av_mark} =  LogUtils.Get Av Log Mark
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark

    # Create slow scanning file
    ${long_scan_file} =  Set Variable  /tmp_test/long_scan_file.exe
    Create Large PE File Of Size  1G  ${long_scan_file}

    # Scan slow scanning file
    ${cls_handle} =  Start Process  ${CLI_SCANNER_PATH}  ${long_scan_file}  stdout=${LOG_FILE}  stderr=STDOUT
    Process Should Be Running   ${cls_handle}
    Register Cleanup    Terminate Process  ${cls_handle}
    wait_for_sophos_threat_detector_log_contains_after_mark  SUSI Libraries loaded  ${td_mark}

    ${ORG_CONTENTS} =  Get File  ${TESTSYSPATH}  encoding_errors=replace
    Append To File  ${TESTSYSPATH}   "#NewLine"

    wait_for_av_log_contains_after_mark  System configuration updated for ${TESTSYSFILE}  ${av_mark}
    check_av_log_does_not_contain_after_mark  System configuration not changed for ${TESTSYSFILE}  ${av_mark}
    wait_for_sophos_threat_detector_log_contains_after_mark  Timed out waiting for graceful shutdown  ${td_mark}  20
    wait_for_sophos_threat_detector_log_contains_after_mark  forcing exit with return code 77  ${td_mark}

    Wait until threat detector running after mark  ${td_mark}


Threat Detector Restarts If System File Contents Change While Multiple Long Scans Are Ongoing

    copy file with permissions  ${TESTSYSPATH}  ${TESTSYSPATHBACKUP}
    Register On Fail   Revert System File To Original

    ${av_mark} =  LogUtils.Get Av Log Mark
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark

    # Create 2 slow scanning files
    ${long_scan_file1} =  Set Variable  /tmp_test/long_scan_file1.exe
    ${long_scan_file2} =  Set Variable  /tmp_test/long_scan_file2.exe
    Create Large PE File Of Size  500M  ${long_scan_file1}
    Create Large PE File Of Size  500M  ${long_scan_file2}

    # Scan the 2 slow scanning files
    ${cls_handle1} =  Start Process  ${CLI_SCANNER_PATH}  ${long_scan_file1}  stdout=${LOG_FILE}  stderr=STDOUT
    ${cls_handle2} =  Start Process  ${CLI_SCANNER_PATH}  ${long_scan_file2}  stdout=${LOG_FILE}  stderr=STDOUT
    Process Should Be Running   ${cls_handle1}
    Process Should Be Running   ${cls_handle2}
    Register Cleanup    Terminate Process  ${cls_handle1}
    Register Cleanup    Terminate Process  ${cls_handle2}
    wait_for_sophos_threat_detector_log_contains_after_mark  SUSI Libraries loaded  ${td_mark}

    ${ORG_CONTENTS} =  Get File  ${TESTSYSPATH}  encoding_errors=replace
    Append To File  ${TESTSYSPATH}   "#NewLine"

    wait_for_av_log_contains_after_mark  System configuration updated for ${TESTSYSFILE}  ${av_mark}
    check_av_log_does_not_contain_after_mark  System configuration not changed for ${TESTSYSFILE}  ${av_mark}
    wait_for_sophos_threat_detector_log_contains_after_mark  Timed out waiting for graceful shutdown  ${td_mark}  20
    wait_for_sophos_threat_detector_log_contains_after_mark  forcing exit with return code 77  ${td_mark}

    Wait until threat detector running after mark  ${td_mark}