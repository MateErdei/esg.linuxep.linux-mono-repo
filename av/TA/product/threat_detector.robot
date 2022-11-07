*** Settings ***
Documentation   Product tests for sophos_threat_detector
Force Tags      PRODUCT  THREAT_DETECTOR

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/LockFile.py
Library         ../Libs/OnFail.py
Library         ../Libs/LogUtils.py

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Test Setup     Threat Detector Test Setup
Test Teardown  Threat Detector Test Teardown

*** Variables ***
${TESTSYSFILE}  hosts
${TESTSYSPATHBACKUP}  /etc/${TESTSYSFILE}backup
${TESTSYSPATH}  /etc/${TESTSYSFILE}
${SOMETIMES_SYMLINKED_SYSFILE}  resolv.conf
${SOMETIMES_SYMLINKED_SYSPATHBACKUP}  /etc/${SOMETIMES_SYMLINKED_SYSFILE}backup
${SOMETIMES_SYMLINKED_SYSPATH}  /etc/${SOMETIMES_SYMLINKED_SYSFILE}


*** Keywords ***

Threat Detector Test Setup
    Component Test Setup
    Mark Sophos Threat Detector Log
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults

Threat Detector Test Teardown
    List AV Plugin Path
    run teardown functions
    Stop AV

    Exclude CustomerID Failed To Read Error
    Exclude Expected Sweep Errors
    Check All Product Logs Do Not Contain Error
    Component Test TearDown

Start AV
    Remove Files   /tmp/threat_detector.stdout  /tmp/threat_detector.stderr
    ${handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}   stdout=/tmp/threat_detector.stdout  stderr=/tmp/threat_detector.stderr
    Set Test Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${handle}

    ${av_log_mark} =  LogUtils.get_av_log_mark
    ${fake_management_log_path} =   FakeManagementLog.get_fake_management_log_path
    ${fake_management_log_mark} =  LogUtils.mark_log_size  ${fake_management_log_path}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed from Marks  ${av_log_mark}  ${fake_management_log_mark}
    # Check AV Plugin Installed checks sophos_threat_detector is started

Stop AV
    ${result} =  Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
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

Threat Detector Log Rotates
    Dump and Reset Logs
    # Ensure the log is created
    Start AV
    Stop AV
    Increase Threat Detector Log To Max Size
    Start AV
    Wait Until Created   ${AV_PLUGIN_PATH}/log/sophos_threat_detector/sophos_threat_detector.log.1   timeout=10s
    Wait Until Sophos Threat Detector Log Contains   UnixSocket
    Stop AV

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/
    Log  ${result.stdout}

    Verify threat detector log rotated

Threat Detector Log Rotates while in chroot
    Dump and Reset Logs
    Register On Fail   Stop AV
    Register On Fail   dump log  ${AV_LOG_PATH}
    Register On Fail   dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail   dump log  ${SUSI_DEBUG_LOG_PATH}

    # Ensure the log is created
    Start AV
    Stop AV
    Increase Threat Detector Log To Max Size   remaining=1024
    Start AV
    Wait Until Created   ${AV_PLUGIN_PATH}/log/sophos_threat_detector/sophos_threat_detector.log.1   timeout=10s
    Wait Until Sophos Threat Detector Log Contains  Process Controller Server starting listening on socket:
    Stop AV

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/
    Log  ${result.stdout}

    Verify threat detector log rotated


Threat detector is killed gracefully
    Dump and Reset Logs

    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark
    Start AV
    Wait until threat detector running  ${td_mark}

    ${cls_handle} =   Start Process  ${CLI_SCANNER_PATH}  /  -x  /mnt/  stdout=${TESTTMP}/cli.log  stderr=STDOUT
    Register Cleanup  Remove File  ${TESTTMP}/cli.log
    Register Cleanup  Dump Log  ${TESTTMP}/cli.log
    Register Cleanup  Terminate Process  ${cls_handle}

    Wait Until Sophos Threat Detector Log Contains  Scan requested of

    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat
    Evaluate  os.kill(${pid}, signal.SIGTERM)  modules=os, signal

    wait_for_log_contains_from_mark  ${td_mark}  Sophos Threat Detector received SIGTERM - shutting down
    wait_for_log_contains_from_mark  ${td_mark}  Sophos Threat Detector is exiting
    wait_for_log_contains_from_mark  ${td_mark}  Exiting Global Susi result = 0
    check_sophos_threat_detector_log_does_not_contain_after_mark  Failed to open lock file  mark=${td_mark}
    check_sophos_threat_detector_log_does_not_contain_after_mark  Failed to acquire lock  mark=${td_mark}
    check_sophos_threat_detector_log_does_not_contain_after_mark  Sophos Threat Detector is exiting with return code 15  mark=${td_mark}

    # Verify SIGTERM is not logged at error level
    Verify Sophos Threat Detector Log Line is informational   Sophos Threat Detector received SIGTERM - shutting down

    Terminate Process  ${cls_handle}
    Stop AV
    Process Should Be Stopped  ${cls_handle}


Threat detector triggers reload on SIGUSR1
    Dump and Reset Logs
    Mark Sophos Threat Detector Log
    Start AV
    Wait until threat detector running
    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat

    Wait Until Sophos Threat Detector Log Contains With Offset  Starting USR1 monitor  timeout=60
    Run Process   /bin/kill   -SIGUSR1   ${pid}

    Wait Until Sophos Threat Detector Log Contains With Offset  Sophos Threat Detector received SIGUSR1 - reloading  timeout=60

    # Verify SIGUSR1 is not logged at error level
    Verify Sophos Threat Detector Log Line is informational   Sophos Threat Detector received SIGUSR1 - reloading

    Stop AV
    Process Should Be Stopped


Threat detector exits if it cannot acquire the susi update lock
    Dump and Reset Logs
#    Register Cleanup    Exclude Failed To Acquire Susi Lock
    Start AV
    Wait until threat detector running
    Wait Until Sophos Threat Detector Log Contains  Process Controller Server starting listening on socket: /var/process_control_socket  timeout=120
    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat

    # Request a scan in order to load SUSI
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${lockfile} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/var/susi_update.lock
    Open And Acquire Lock   ${lockfile}
    Register Cleanup  Release Lock

    Run Process   /bin/kill   -SIGUSR1   ${pid}

    Wait Until Sophos Threat Detector Log Contains  Reload triggered by USR1
    Wait Until Sophos Threat Detector Log Contains  Failed to acquire lock on ${lockfile}  timeout=120
    Wait Until Sophos Threat Detector Log Contains  UnixSocket <> Closing Scanning Server socket

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Threat Detector Not Running


Threat Detector Logs Susi Version when applicable
    Dump and Reset Logs
    Start AV
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Wait Until Sophos Threat Detector Log Contains With Offset  Initializing SUSI
    Wait Until Sophos Threat Detector Log Contains With Offset  SUSI Libraries loaded:
    mark sophos threat detector log

    ${rc2}   ${output2} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Wait Until Sophos Threat Detector Log Contains With Offset  SUSI already initialised
    Sleep  1s  Allow a second for Threat Detector to log the loading of SUSI Libraries
    threat detector log should not contain with offset  SUSI Libraries loaded:
    mark sophos threat detector log

    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -SIGUSR1   ${pid}
    Wait Until Sophos Threat Detector Log Contains With Offset  Threat scanner is already up to date
    Sleep  1s  Allow a second for Threat Detector to log the loading of SUSI Libraries
    threat detector log should not contain with offset  SUSI Libraries loaded:


Threat Detector Doesnt Log Every Scan
    Dump and Reset Logs
    Register On Fail   dump log  ${SUSI_DEBUG_LOG_PATH}
    Set Log Level  INFO
    Register Cleanup       Set Log Level  DEBUG
    Remove File    ${SUSI_DEBUG_LOG_PATH}
    Start AV

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Wait Until Sophos Threat Detector Log Contains With Offset  Initializing SUSI
    Sleep  1s  Allow a second for Threat Detector to log the starting and finishing of the scan
    check log does not contain   Starting scan of    ${SUSI_DEBUG_LOG_PATH}  Susi Debug Log
    check log does not contain   Finished scanning   ${SUSI_DEBUG_LOG_PATH}  Susi Debug Log
    dump log  ${SUSI_DEBUG_LOG_PATH}
