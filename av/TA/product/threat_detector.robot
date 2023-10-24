*** Settings ***
Documentation   Product tests for sophos_threat_detector
Force Tags      PRODUCT  THREAT_DETECTOR    TAP_PARALLEL4

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FileSampleObfuscator.py
Library         ../Libs/JsonUtils.py
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
    Set Test Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${None}
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Component Test Setup
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Create File  ${COMPONENT_ROOT_PATH}/var/customer_id.txt  c1cfcf69a42311a6084bcefe8af02c8a
    Create File  ${COMPONENT_ROOT_PATH_CHROOT}/var/customer_id.txt  c1cfcf69a42311a6084bcefe8af02c8a

Threat Detector Test Teardown
    List AV Plugin Path
    run teardown functions
    Stop AV

    Exclude CustomerID Failed To Read Error
    Exclude Expected Sweep Errors
    Check All Product Logs Do Not Contain Error
    Component Test TearDown

Start Threat Detector As Limited User
    Run Process    chmod  -R  a+rwX
    ...  ${COMPONENT_ROOT_PATH}/chroot/log  ${COMPONENT_ROOT_PATH}/log
    ...  ${COMPONENT_ROOT_PATH}/chroot/var  ${COMPONENT_ROOT_PATH}/chroot/tmp
    ...  ${COMPONENT_ROOT_PATH}/chroot/etc
    Run Process   chown  -R  sophos-spl-threat-detector:sophos-spl-group  ${COMPONENT_ROOT_PATH}/chroot
    ${result} =  Run Process  ls  -alR  ${COMPONENT_ROOT_PATH}/chroot/etc  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}  stderr=STDOUT
    Log  ${result.stdout}
    Remove Files   /tmp/threat_detector.stdout  /tmp/threat_detector.stderr
    ${threat_detector_handle} =  Start Process  runuser  -u  sophos-spl-threat-detector  -g  sophos-spl-group  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}
    ...  stdout=/tmp/threat_detector.stdout  stderr=/tmp/threat_detector.stderr

    register on fail  dump log  /tmp/threat_detector.stdout
    register on fail  dump log  /tmp/threat_detector.stderr
    Set Test Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${threat_detector_handle}

Start Threat Detector As Root
    Remove Files   /tmp/threat_detector.stdout  /tmp/threat_detector.stderr
    ${threat_detector_handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}
    ...  stdout=/tmp/threat_detector.stdout  stderr=/tmp/threat_detector.stderr
    register on fail  dump log  /tmp/threat_detector.stdout
    register on fail  dump log  /tmp/threat_detector.stderr
    Set Test Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${threat_detector_handle}

Start AV Plugin
    ${fake_management_log_path} =   FakeManagementLog.get_fake_management_log_path
    ${fake_management_log_mark} =  LogUtils.mark_log_size  ${fake_management_log_path}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed from Marks  ${fake_management_log_mark}
    # Check AV Plugin Installed checks sophos_threat_detector is started

Start AV
    Start Threat Detector As Root
    Start AV Plugin

Stop Threat Detector
    return from keyword if  ${THREAT_DETECTOR_PLUGIN_HANDLE} == ${None}
    ${result} =    Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    Log    ${result.rc}
    Log    ${result.stdout}
    Log    ${result.stderr}

Stop AV Plugin
    return from keyword if  ${AV_PLUGIN_HANDLE} == ${None}
    Terminate Process  ${AV_PLUGIN_HANDLE}

Stop AV
    Stop Threat Detector
    Stop AV Plugin

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
    Wait Until Sophos Threat Detector Log Contains  ProcessControlServer starting listening on socket:
    Stop AV

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/
    Log  ${result.stdout}

    Verify threat detector log rotated


Threat detector is killed gracefully
    Dump and Reset Logs

    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark
    Start AV
    Wait until threat detector running after mark  ${td_mark}

    ${cls_handle} =   Start Process  ${CLI_SCANNER_PATH}  /  -x  /mnt/  stdout=${TESTTMP}/cli.log  stderr=STDOUT
    Register Cleanup  Remove File  ${TESTTMP}/cli.log
    Register Cleanup  Dump Log  ${TESTTMP}/cli.log
    Register Cleanup  Terminate Process  ${cls_handle}

    Wait Until Sophos Threat Detector Log Contains  ScanningServerConnectionThread scan requested of

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
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark
    Start AV
    Wait until threat detector running after mark    ${td_mark}
    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat

    Wait For Sophos Threat Detector Log Contains After Mark  Starting USR1 monitor  ${td_mark}  timeout=60
    Run Process   /bin/kill   -SIGUSR1   ${pid}

    Wait For Sophos Threat Detector Log Contains After Mark  Sophos Threat Detector received SIGUSR1 - reloading  ${td_mark}  timeout=60

    # Verify SIGUSR1 is not logged at error level
    Verify Sophos Threat Detector Log Line is informational   Sophos Threat Detector received SIGUSR1 - reloading

    Stop AV
    Process Should Be Stopped


Threat detector exits if it cannot acquire the susi update lock
#   TODO: LINUXDAR-5692 re-enable when bug gets fixed, or when we have a better understanding of the issue
    [Tags]  disabled
    Dump and Reset Logs
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark
#    Register Cleanup    Exclude Failed To Acquire Susi Lock
    Start AV
    Wait until threat detector running after mark    ${td_mark}
    Wait Until Sophos Threat Detector Log Contains  ProcessControlServer starting listening on socket: /var/process_control_socket  timeout=120
    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat

    # Request a scan in order to load SUSI
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${lockfile} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/var/susi_update.lock
    Open And Acquire Lock   ${lockfile}
    Register Cleanup  Release Lock

    Run Process   /bin/kill   -SIGUSR1   ${pid}

    Wait For Sophos Threat Detector Log Contains After Mark  Reload triggered by USR1  ${td_mark}
    Wait For Sophos Threat Detector Log Contains After Mark  Failed to acquire lock on ${lockfile}  ${td_mark}  timeout=120
    Wait For Sophos Threat Detector Log Contains After Mark  UnixSocket <> Closing ScanningServer socket  ${td_mark}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Threat Detector Not Running


Threat Detector Logs Susi Version when applicable
    Dump and Reset Logs
    ${td_mark} =  Get Sophos Threat Detector Log Mark
    Start AV
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Wait For Sophos Threat Detector Log Contains After Mark  Initializing SUSI  ${td_mark}
    Wait For Sophos Threat Detector Log Contains After Mark  SUSI Libraries loaded:  ${td_mark}
    ${td_mark2} =  Get Sophos Threat Detector Log Mark

    ${rc2}   ${output2} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Wait For Sophos Threat Detector Log Contains After Mark  SUSI already initialised  ${td_mark2}
    Sleep  1s  Allow a second for Threat Detector to log the loading of SUSI Libraries
    Check Sophos Threat Detector Log Does Not Contain After Mark  SUSI Libraries loaded:  ${td_mark2}
    ${td_mark3} =  Get Sophos Threat Detector Log Mark

    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -SIGUSR1   ${pid}
    Wait For Sophos Threat Detector Log Contains After Mark  Threat scanner is already up to date  ${td_mark3}
    Sleep  1s  Allow a second for Threat Detector to log the loading of SUSI Libraries
    Check Sophos Threat Detector Log Does Not Contain After Mark  SUSI Libraries loaded:  ${td_mark3}


Threat Detector Doesnt Log Every Scan
    Dump and Reset Logs
    ${td_mark} =  Get Sophos Threat Detector Log Mark
    Register On Fail   dump log  ${SUSI_DEBUG_LOG_PATH}
    Set Log Level  INFO
    Register Cleanup       Set Log Level  DEBUG
    Remove File    ${SUSI_DEBUG_LOG_PATH}
    Start AV

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Wait For Sophos Threat Detector Log Contains After Mark  Initializing SUSI  ${td_mark}
    Sleep  1s  Allow a second for Threat Detector to log the starting and finishing of the scan
    check log does not contain   Starting scan of    ${SUSI_DEBUG_LOG_PATH}  Susi Debug Log
    check log does not contain   Finished scanning   ${SUSI_DEBUG_LOG_PATH}  Susi Debug Log
    dump log  ${SUSI_DEBUG_LOG_PATH}

Threat Detector can have Machine Learning Turned off
    Register Cleanup   dump log  ${SUSI_DEBUG_LOG_PATH}
    Register Cleanup   dump log  ${AV_LOG_PATH}
    Register Cleanup   dump log  ${THREAT_DETECTOR_LOG_PATH}

    # Set CORE policy to turn off ML detections
    set_default_policy_from_file  CORE  ${RESOURCES_PATH}/core_policy/CORE-36_ml_disabled.xml
    Copy File   ${RESOURCES_PATH}/susi_settings/susi_settings_ml_off.json  ${SUSI_STARTUP_SETTINGS_FILE}
    Copy File   ${RESOURCES_PATH}/susi_settings/susi_settings_ml_off.json  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    # Try scanning MLengHighScore
    Start AV

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Register On Fail  Log  ${output}

    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Not Contain  ${output}  Detected "${NORMAL_DIRECTORY}/MLengHighScore.exe"

Threat Detector loads proxy from config file
    # Create proxy file
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy  {"proxy":"localhost:8080"}

    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark
    Register On Fail  dump marked log  ${THREAT_DETECTOR_LOG_PATH}  ${td_mark}

    Start AV
    wait for log contains from mark  ${td_mark}  LiveProtection will use http://localhost:8080 for SXL4 connections


Sophos Threat Detector sets default if susi startup settings permissions ownership incorrect
    [Tags]  FAULT INJECTION
    Exclude MachineID Permission Error
    Exclude Threat Report Server died
    Threat Detector Failed to Copy
    # Not interested to SXL4/GlobalRep errors
    Exclude Globalrep errors

    # Doesn't matter what the files contain if we are going to block access
    create file  ${SUSI_STARTUP_SETTINGS_FILE}
    create file  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    Run Process  chmod  000  ${SUSI_STARTUP_SETTINGS_FILE}
    Run Process  chmod  000  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}
    Register Cleanup   Remove File   ${SUSI_STARTUP_SETTINGS_FILE}
    Register Cleanup   Remove File   ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    ${threat_detector_mark3} =  Get Sophos Threat Detector Log Mark
    Start Threat Detector As Limited User

    # scan eicar to trigger susi to be loaded
    Check avscanner can detect eicar  ${CLI_SCANNER_PATH}

    Wait For Sophos Threat Detector Log Contains After Mark   Turning Live Protection off as default - could not read SUSI settings  ${threat_detector_mark3}
