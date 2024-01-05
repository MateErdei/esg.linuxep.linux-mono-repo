*** Settings ***
Documentation    Integration tests of sophos_threat_detector
Force Tags       INTEGRATION  SOPHOS_THREAT_DETECTOR  TAP_PARALLEL3

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/SafeStoreResources.robot

Library         ${COMMON_TEST_LIBS}/CoreDumps.py
Library         ../Libs/OnFail.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ProcessUtils.py
Library         ../Libs/FileSampleObfuscator.py
Library         ../Libs/FileUtils.py

Suite Setup     AVSophosThreatDetector Suite Setup
Suite Teardown  AVSophosThreatDetector Suite TearDown

Test Setup      AVSophosThreatDetector Test Setup
Test Teardown   AVSophosThreatDetector Test TearDown

*** Variables ***
${CLEAN_STRING}     not an eicar
${CUSTOMERID_FILE}  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/var/customer_id.txt
${MACHINEID_CHROOT_FILE}  ${COMPONENT_ROOT_PATH}/chroot${SOPHOS_INSTALL}/base/etc/machine_id.txt
${MACHINEID_FILE}   ${SOPHOS_INSTALL}/base/etc/machine_id.txt

*** Test Cases ***
Test Global Rep works in chroot
    Restart Sophos Threat Detector
    ${mark} =  Mark Log Size    ${SUSI_DEBUG_LOG_PATH}
    scan GR test file
    check sophos_threat_dector log for successful global rep lookup  ${mark}

Sophos Threat Detector Has No Unnecessary Capabilities
    # ensure that threat_detector is fully started up
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart Sophos Threat Detector
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket: /var/process_control_socket
    ...   ${threat_detector_mark}
    ...   timeout=60

    ${rc}   ${pid} =       Run And Return Rc And Output    pgrep sophos_threat
    ${rc}   ${output} =    Run And Return Rc And Output    getpcaps ${pid}
    Should Not Contain  ${output}  cap_sys_chroot
    # Handle different format of the output from getpcaps on Ubuntu 20.04
    IF  ("${output}" != "Capabilities for `${pid}\': =") and ("${output}" != "${pid}: =")
        Fail  msg=Unexpected capabilities: ${output}
    END


Threat detector does not recreate logging symlink if present
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Should Exist   ${CHROOT_LOGGING_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}/sophos_threat_detector.log
    Restart Sophos Threat Detector
    Check Sophos Threat Detector Log Does Not Contain After Mark   LogSetup <> Create symlink for logs at  ${threat_detector_mark}
    Check Sophos Threat Detector Log Does Not Contain After Mark   LogSetup <> Failed to create symlink for logs at  ${threat_detector_mark}

Threat detector recreates logging symlink if missing
    register cleanup   Exclude STD Symlink Error
    register cleanup   Install With Base SDDS
    Should Exist   ${CHROOT_LOGGING_SYMLINK}

    Run Process   rm   ${CHROOT_LOGGING_SYMLINK}
    Should Not Exist   ${CHROOT_LOGGING_SYMLINK}
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart Sophos Threat Detector

    Wait For Sophos Threat Detector Log Contains After Mark  LogSetup <> Create symlink for logs at  ${threat_detector_mark}
    Check Sophos Threat Detector Log Does Not Contain After Mark   LogSetup <> Failed to create symlink for logs at  ${threat_detector_mark}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}/sophos_threat_detector.log

    Should not exist   ${AV_PLUGIN_PATH}/chroot/log/testfile
    Create file   ${CHROOT_LOGGING_SYMLINK}/testfile
    Should exist   ${AV_PLUGIN_PATH}/chroot/log/testfile

Threat detector aborts if logging symlink cannot be created
    register cleanup   Exclude Failed To Create Symlink
    register cleanup   Install With Base SDDS
    Should Exist   ${CHROOT_LOGGING_SYMLINK}

    Run Process   rm   ${CHROOT_LOGGING_SYMLINK}
    # stop sophos_threat_detector from creating the link, by denying group access to the directory
    Run Process   chmod   g-rwx    ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart Sophos Threat Detector

    Wait For Sophos Threat Detector Log Contains After Mark   LogSetup <> Failed to create symlink for logs at  ${threat_detector_mark}
    Check Sophos Threat Detector Log Does Not Contain After Mark   LogSetup <> Create symlink for logs at  ${threat_detector_mark}
    Should Not Exist   ${CHROOT_LOGGING_SYMLINK}

Threat Detector Restarts When /etc/hosts changed
    # ensure that both AV and threat_detector are ready
    Wait_For_Log_contains_after_last_restart  ${AV_LOG_PATH}  Starting sophos_threat_detector monitor
    Wait_For_Log_contains_after_last_restart  ${THREAT_DETECTOR_LOG_PATH}
    ...  ProcessControlServer starting listening on socket: /var/process_control_socket  timeout=${120}

    ${SOPHOS_THREAT_DETECTOR_PID_AT_START} =  Get Sophos Threat Detector PID From File

    ${av_mark} =  LogUtils.Get Av Log Mark
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark

    Alter Hosts

    # wait for AV log
    wait_for_av_log_contains_after_mark  Restarting sophos_threat_detector as the system configuration has changed  mark=${av_mark}

    # Ideally we'd wait for the log while checking that either the process is running or shutdown file exists

    # Can't check the shutdown file - ThreatDetector restarts too quickly
    # Wait Until Sophos Threat Detector Shutdown File Exists

    # Should have restarted almost immediately, but less than 10 seconds mean we've started faster than the normal watchdog timeout
    # Unfortunately TA machines seem to be slower that my test VM
    wait_for_log_contains_from_mark  ${td_mark}  ProcessControlServer starting listening on socket: /var/process_control_socket  timeout=8
    Wait until threat detector running after mark  ${td_mark}

    ${SOPHOS_THREAT_DETECTOR_PID_AT_END} =  Get Sophos Threat Detector PID From File
    Should Not Be Equal As Integers  ${SOPHOS_THREAT_DETECTOR_PID_AT_START}  ${SOPHOS_THREAT_DETECTOR_PID_AT_END}

Threat Detector restarts if no scans requested within the configured timeout
    Stop sophos_threat_detector
    Create File  ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config  {"shutdownTimeout":15}
    Register Cleanup   Remove File   ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config
    Register Cleanup   Stop sophos_threat_detector

    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark
    Start sophos_threat_detector
    wait_for_log_contains_from_mark  ${td_mark}  ProcessControlServer starting listening on socket: /var/process_control_socket  timeout=120
    wait_for_log_contains_from_mark  ${td_mark}  Default shutdown timeout set to 15 seconds.
    wait_for_log_contains_from_mark  ${td_mark}  Setting shutdown timeout to

    ${SOPHOS_THREAT_DETECTOR_PID_AT_START} =  Get Sophos Threat Detector PID From File
    ${td_mark} =  LogUtils.Get Sophos Threat Detector Log Mark

    Force SUSI to be initialized

    # No scans requested for ${timeout} seconds - shutting down.
    wait_for_log_contains_from_mark  ${td_mark}  No scans requested  timeout=20
    ${td_mark2} =  wait_for_log_contains_from_mark  ${td_mark}  Sophos Threat Detector is exiting

    # TD should restart immediately:
    wait_for_log_contains_from_mark  ${td_mark2}  Logger av configured for level
    wait_for_log_contains_from_mark  ${td_mark2}  ProcessControlServer starting listening on socket: /var/process_control_socket  timeout=${15}
    wait_for_log_contains_from_mark  ${td_mark2}  Default shutdown timeout set to 15 seconds.
    wait_for_log_contains_from_mark  ${td_mark2}  Setting shutdown timeout to
    Wait until threat detector running after mark  ${td_mark2}

    ${SOPHOS_THREAT_DETECTOR_PID_AT_END} =  Get Sophos Threat Detector PID From File
    Should Not Be Equal As Integers  ${SOPHOS_THREAT_DETECTOR_PID_AT_START}  ${SOPHOS_THREAT_DETECTOR_PID_AT_END}

Threat Detector prolongs timeout if a scan is requested within the configured timeout
    Stop sophos_threat_detector
    Create File  ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config  {"shutdownTimeout":15}
    Register Cleanup   Remove File   ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config
    Register Cleanup   Stop sophos_threat_detector

    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Start sophos_threat_detector
    Wait For Sophos Threat Detector Log Contains After Mark  ProcessControlServer starting listening on socket: /var/process_control_socket  ${threat_detector_mark}  timeout=120
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Wait For Sophos Threat Detector Log Contains After Mark  Default shutdown timeout set to 15 seconds.  ${threat_detector_mark}
    Wait For Sophos Threat Detector Log Contains After Mark  Setting shutdown timeout to  ${threat_detector_mark}


    ${threat_detector_mark2} =  Get Sophos Threat Detector Log Mark
    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    # Scan requested less than ${timeout1} seconds ago - continuing
    Wait For Sophos Threat Detector Log Contains After Mark  Scan requested less than  ${threat_detector_mark}  timeout=20
    Wait For Sophos Threat Detector Log Contains After Mark  Setting shutdown timeout to  ${threat_detector_mark2}

    # No scans requested for ${timeout2} seconds - shutting down.
    Wait For Sophos Threat Detector Log Contains After Mark  No scans requested for  ${threat_detector_mark2}  timeout=20
    Wait For Sophos Threat Detector Log Contains After Mark  Sophos Threat Detector is exiting  ${threat_detector_mark2}
    Wait For Sophos Threat Detector Log Contains After Mark  ProcessControlServer starting listening on socket: /var/process_control_socket  ${threat_detector_mark2}  timeout=120
    Check Sophos Threat Detector has different PID  ${SOPHOS_THREAT_DETECTOR_PID}

    Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace

Threat Detector Is Given Non-Permission EndpointId
    [Tags]  FAULT INJECTION
    register cleanup  Exclude MachineID Permission Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}  ab7b6758a3ab11ba8a51d25aa06d1cf4
    Run Process  chmod  000  ${MACHINEID_FILE}
    Register Cleanup  Create Default Machine Id
    Register Cleanup  Remove File  ${MACHINEID_FILE}
    Register Cleanup    Stop Sophos_threat_detector
    Remove File  ${MACHINEID_CHROOT_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  Failed to copy: "${MACHINEID_FILE}"  ${threat_detector_mark}
    Check Sophos Threat Detector Log Contains After Mark  Failed to read machine ID - using default value  ${threat_detector_mark}

SUSI Is Given Non-Permission EndpointId
    [Tags]  FAULT INJECTION
    register cleanup  Exclude MachineID Permission Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}  ab7b6758a3ab11ba8a51d25aa06d1cf4
    Register Cleanup  Create Default Machine Id
    Register Cleanup  Remove File  ${MACHINEID_FILE}
    Start AV Plugin And Threat Detector
    Wait until threat detector running after mark  ${threat_detector_mark}
    Run Process  chmod  000  ${MACHINEID_CHROOT_FILE}
    Register Cleanup    Remove File  ${MACHINEID_CHROOT_FILE}
    Register Cleanup    Stop Sophos_threat_detector
    Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  Failed to read machine ID - using default value  ${threat_detector_mark}

SUSI Is Given Empty EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude EndpointID Cannot Be Empty
    register cleanup    Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID cannot be empty  ${threat_detector_mark}


SUSI Is Given A New Line As EndpointId
    [Tags]  FAULT INJECTION
    register cleanup     Exclude MachineID New Line Error
    register cleanup     Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}  \n
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID cannot contain a new line  ${threat_detector_mark}

SUSI Is Given An Empty Space As EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Empty Space Error
    register cleanup    Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}  ${SPACE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID cannot contain a empty space  ${threat_detector_mark}

SUSI Is Given Short EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Shoulb Be 32 Hex Error
    register cleanup    Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}  d22829d94b76c016ec4e04b08baef
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID should be 32 hex characters  ${threat_detector_mark}

SUSI Is Given Long EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Shoulb Be 32 Hex Error
    register cleanup    Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}  d22829d94b76c016ec4e04b08baefaaaaaaaaaaaaaaa
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID should be 32 hex characters  ${threat_detector_mark}

SUSI Is Given Non-hex EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Hex Format Error
    register cleanup    Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}  GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID must be in hex format  ${threat_detector_mark}

SUSI Is Given Non-UTF As EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Hex Format Error
    register cleanup    Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    ${rc}   ${output} =    Run And Return Rc And Output  echo -n "ソフォスソフォスソフォスソフォス" | iconv -f utf-8 -t euc-jp > ${MACHINEID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID must be in hex format  ${threat_detector_mark}

SUSI Is Given Binary EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Failed To Read Error
    register cleanup    Exclude MachineID New Line Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Copy File  /bin/true  ${MACHINEID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  Failed to read machine ID - using default value  ${threat_detector_mark}

SUSI Is Given Large File EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Shoulb Be 32 Hex Error
    register cleanup    Exclude MachineID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${MACHINEID_FILE}

    FOR  ${item}  IN RANGE  1  10
        Append To File  ${MACHINEID_FILE}  ab7b6758a3ab11ba8a51d25aa06d1cf4
    END

    ${idfile} =    Get File  ${MACHINEID_FILE}
    @{list} =    Split to lines  ${idfile}

    FOR  ${line}  IN  @{list}
       Log  ${line}
    END

    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  EndpointID should be 32 hex characters  ${threat_detector_mark}

SUSI Is Given Empty CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Cannot Be Empty Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID cannot be empty  ${threat_detector_mark}

SUSI Is Given A New Line As CustomerId
    [Tags]  FAULT INJECTION
    register cleanup   Exclude CustomerID New Line Error
    register cleanup   Exclude CustomerID Hex Format Error
    register cleanup   Exclude CustomerID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}  \n
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID cannot contain a new line  ${threat_detector_mark}

SUSI Is Given An Empty Space As CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Cannot Be Empty Space Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}  ${SPACE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID cannot contain a empty space  ${threat_detector_mark}

SUSI Is Given Short CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baef
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID should be 32 hex characters  ${threat_detector_mark}

SUSI Is Given Long CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baefaaaaaaaaaaaaaaa
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID should be 32 hex characters  ${threat_detector_mark}

SUSI Is Given Non-hex CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}  GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID must be in hex format  ${threat_detector_mark}

SUSI Is Given Non-UTF As CustomerId
    [Tags]  FAULT INJECTION
    register cleanup   Exclude CustomerID Hex Format Error
    register cleanup   Exclude CustomerID Hex Format Error
    register cleanup   Exclude CustomerID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    ${rc}   ${output} =    Run And Return Rc And Output  echo -n "ソフォスソフォスソフォスソフォス" | iconv -f utf-8 -t euc-jp > ${CUSTOMERID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID must be in hex format  ${threat_detector_mark}

SUSI Is Given Binary CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID New Line Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Copy File  /bin/true  ${CUSTOMERID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  Failed to read customerID - using default value  ${threat_detector_mark}

SUSI Is Given Large File CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}

    FOR  ${item}  IN RANGE  1  10
        Append To File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baeffaa
    END

    ${idfile} =    Get File  ${CUSTOMERID_FILE}
    @{list} =    Split to lines  ${idfile}

    FOR  ${line}  IN  @{list}
       Log  ${line}
    END
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  CustomerID should be 32 hex characters  ${threat_detector_mark}

SUSI Is Given Non-Permission CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Failed To Read Error
    register cleanup    Exclude CustomerID Hex Format Error
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Stop sophos_threat_detector
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baeffaa
    Run Process  chmod  000  ${CUSTOMERID_FILE}
    Register Cleanup    Remove File  ${CUSTOMERID_FILE}
    Register Cleanup    Stop Sophos_threat_detector
    Start sophos_threat_detector and Force SUSI to be initialized
    Check Sophos Threat Detector Log Contains After Mark  Failed to read customerID - using default value  ${threat_detector_mark}

Threat Detector Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    register cleanup    Exclude Watchdog Log Unable To Open File Error
    Create Temporary eicar in  ${NORMAL_DIRECTORY}/naughty_eicar

    ${av_mark} =  Get AV Log Mark
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar
    Check Sophos Threat Detector Log Contains After Mark  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/naughty_eicar (On Demand)  ${threat_detector_mark}

    Run  chmod 444 ${THREAT_DETECTOR_LOG_PATH}
    Register Cleanup  Run  chmod 600 ${THREAT_DETECTOR_LOG_PATH}
    Register Cleanup  Stop Sophos_threat_detector

    # use the basic restart method, as threat detector will not be logging
    restart sophos_threat_detector

    ${result} =  Run Process  ls  -l  ${THREAT_DETECTOR_LOG_PATH}
    Log  New permissions: ${result.stdout}

    ${av_mark2} =  Get AV Log Mark
    ${threat_detector_mark2} =  Get Sophos Threat Detector Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar

    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark2}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar
    Check Sophos Threat Detector Log Does Not Contain After Mark  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/naughty_eicar  ${threat_detector_mark2}


SUSI Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    register cleanup    Exclude Watchdog Log Unable To Open File Error
    Create Temporary eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    register on fail  dump log  ${AV_LOG_PATH}

    ${av_mark} =  Get AV Log Mark
    ${susi_debug_mark} =  Get SUSI Debug Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar
    Check SUSI Debug Log Contains After Mark  SUSI scan for ${NORMAL_DIRECTORY}/naughty_eicar  ${susi_debug_mark}

    Run  chmod 444 ${SUSI_DEBUG_LOG_PATH}
    Register Cleanup  Stop Sophos_threat_detector
    Register Cleanup  Run  chmod 600 ${SUSI_DEBUG_LOG_PATH}

    ${av_mark2} =  Get AV Log Mark
    ${susi_debug_mark2} =  Get SUSI Debug Log Mark
    Restart Sophos Threat Detector

    ${result} =  Run Process  ls  -l  ${SUSI_DEBUG_LOG_PATH}
    Log  New permissions: ${result.stdout}

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark2}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar
    Check SUSI Debug Log Does Not Contain After Mark  SUSI scan for ${NORMAL_DIRECTORY}/naughty_eicar  ${susi_debug_mark2}

SUSI Debug Log Does Not Contain Info Level Logs By Default
    register cleanup     Exclude Watchdog Log Unable To Open File Error
    register cleanup     Stop sophos_threat_detector
    register cleanup     Set Log Level  DEBUG
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    ${susi_debug_mark} =  Get SUSI Debug Log Mark

    Create Temporary eicar in  ${NORMAL_DIRECTORY}/eicar.com
    Set Log Level  INFO

    Restart Sophos Threat Detector

    # Request a scan in order to load SUSI
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Wait Until File exists  ${SUSI_DEBUG_LOG_PATH}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar.com
    Wait For Sophos Threat Detector Log Contains After Mark  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/eicar.com (On Demand)  ${threat_detector_mark}

    # Confirm that no info-level SUSI log messages were printed (ie. those starting with "I")
    ${contents} =  Get SUSI Debug Log After Mark   ${susi_debug_mark}
    @{lines}=  Split to lines  ${contents}
    FOR  ${line}  IN  @{lines}
        Log  ${line}
        ${lineStr} =  Convert To String  ${line}
        Should Not Start With  ${lineStr}  I
    END

Sophos Threat Detector Is Shutdown On LiveProtection Change
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    Force SUSI to be initialized

    # restart AV to force policy to be applied & sent to threat detector
    Stop AV Plugin

    ${revid} =   Generate Random String
    ${policyContent} =   create_corc_policy  revid=${revid}  sxlLookupEnabled=${true}
    Send CORC Policy To Base From Content  ${policyContent}

    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    ${av_mark} =  Get Av Log Mark

    Start AV Plugin And Threat Detector
    wait_for_log_contains_from_mark  ${av_mark}  Restarting sophos_threat_detector as the configuration has changed
    ${threat_detector_mark}=  Wait For Sophos Threat Detector Log Contains After Mark  Sophos Threat Detector is restarting to pick up changed   ${threat_detector_mark}
    wait_for_log_contains_from_mark  ${threat_detector_mark}  Logger av configured for level:

    ${susi_debug_mark} =  Get SUSI Debug Log Mark
    Check avscanner can detect eicar
    Wait For SUSI Debug Log Contains After Mark    "enableLookup" : true  ${susi_debug_mark}

    Check Sophos Threat Detector Has Different PID  ${SOPHOS_THREAT_DETECTOR_PID}
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    ${susi_debug_mark} =  Get SUSI Debug Log Mark
    ${av_mark} =  Get Av Log Mark

    ${revid} =   Generate Random String
    ${policyContent} =   create_corc_policy  revid=${revid}  sxlLookupEnabled=${false}
    Send CORC Policy To Base From Content  ${policyContent}

    wait_for_log_contains_from_mark  ${av_mark}  Restarting sophos_threat_detector as the configuration has changed
    ${threat_detector_mark}=  Wait For Sophos Threat Detector Log Contains After Mark  Sophos Threat Detector is restarting to pick up changed   ${threat_detector_mark}
    wait_for_log_contains_from_mark  ${threat_detector_mark}  Logger av configured for level:
    Check avscanner can detect eicar
    Wait For SUSI Debug Log Contains After Mark    "enableLookup" : false  ${susi_debug_mark}

    Check Sophos Threat Detector Has Different PID  ${SOPHOS_THREAT_DETECTOR_PID}

Sophos Threat Detector Is Ignoring Reload Request
    # Disable on-access
    ${policyContent} =   get_complete_core_policy  on_access_enabled=${false}
    ${av_mark} =  Get Av Log Mark
    Send CORE Policy To Base From Content    ${policyContent}
    ${av_mark} =  Wait For Log Contains From Mark    ${av_mark}      Received CORE policy
    ${av_mark} =  Wait For Log Contains From Mark    ${av_mark}      Processing CORE policy

    #unload susi
    Stop sophos_threat_detector
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Start sophos_threat_detector

    ${threat_detector_mark} =  Wait For Log Contains From Mark    ${threat_detector_mark}   ProcessControlServer starting listening on socket: /var/process_control_socket

    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    ${av_mark} =  Get Av Log Mark

    # Send a policy that won't cause a restart
    ${policyContent} =  create_sav_policy  pua_exclusions=PsExec
    Log  SAV Policy=${policyContent}
    Send SAV Policy To Base From Content  ${policyContent}
    
    ${av_mark} =  Wait For Log Contains From Mark    ${av_mark}      Received SAV policy
    ${av_mark} =  Wait For Log Contains From Mark    ${av_mark}      Processing SAV policy

    Wait For Sophos Threat Detector Log Contains After Mark  Skipping susi reload because susi is not initialised  ${threat_detector_mark}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

Sophos Threat Detector Scans Archive With Error And Eicar And Reports Successfully
    Register Cleanup  Exclude As Password Protected
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/scanErrorAndThreat.tar  ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar
    ${archive_sha} =  Get SHA256  ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar  --scan-archives
    wait_for_log_contains_from_mark  ${td_mark}  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar/eicar.com (On Demand)
    wait_for_log_contains_from_mark  ${td_mark}  Failed to scan ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar/EncryptedSpreadsheet.xlsx as it is password protected
    wait_for_log_contains_from_mark  ${td_mark}  Calculated the SHA256 of ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar: ${archive_sha}
    wait_for_log_contains_from_mark  ${td_mark}  Sending report for detection 'EICAR-AV-Test' in ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar
    Wait Until AV Plugin Log Contains Detection Event XML After Mark
    ...  mark=${av_mark}
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=${archive_sha}
    ...  path=${NORMAL_DIRECTORY}/scanErrorAndThreat.tar

Sophos Threat Detector Scans Archive With Multiple Threats And Reports Successfully
    ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
    Create Directory  ${ARCHIVE_DIR}
    Create File  ${ARCHIVE_DIR}/1_dsa    ${DSA_BY_NAME_STRING}
    Create File  ${ARCHIVE_DIR}/2_eicar  ${EICAR_STRING}
    Run Process  tar  --mtime\=UTC 2022-01-01  -C  ${ARCHIVE_DIR}  -cf  ${NORMAL_DIRECTORY}/test.tar  1_dsa  2_eicar
    ${dsa_sha} =  Get SHA256  ${ARCHIVE_DIR}/1_dsa
    Remove Directory  ${ARCHIVE_DIR}  recursive=True
    ${archive_sha} =  Get SHA256  ${NORMAL_DIRECTORY}/test.tar
    Should Not Be Equal As Strings  ${dsa_sha}  ${archive_sha}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/test.tar  --scan-archives
    wait_for_log_contains_from_mark  ${td_mark}  Detected "Troj/TestSFS-G" in ${NORMAL_DIRECTORY}/test.tar/1_dsa (On Demand)
    wait_for_log_contains_from_mark  ${td_mark}  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/test.tar/2_eicar (On Demand)
    wait_for_log_contains_from_mark  ${td_mark}  Calculated the SHA256 of ${NORMAL_DIRECTORY}/test.tar: ${archive_sha}
    wait_for_log_contains_from_mark  ${td_mark}  Sending report for detection 'Troj/TestSFS-G' in ${NORMAL_DIRECTORY}/test.tar
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark}  Troj/TestSFS-G  ${NORMAL_DIRECTORY}/test.tar
    Wait Until AV Plugin Log Contains Detection Event XML After Mark
    ...  mark=${av_mark}
    ...  user_id=n/a
    ...  name=Troj/TestSFS-G
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=${archive_sha}
    ...  path=${NORMAL_DIRECTORY}/test.tar


Sophos Threat Detector Gives Different Threat Id Depending On Path And Sha And Is Reproducible
    Create File  ${NORMAL_DIRECTORY}/eicar.com  ${EICAR_STRING}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar.com
    wait_for_log_contains_from_mark  ${td_mark}  Threat ID: e52cf957-a0dc-5b12-bad2-561197a5cae4
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar.com
    wait_for_log_contains_from_mark  ${td_mark}  Threat ID: e52cf957-a0dc-5b12-bad2-561197a5cae4
    Remove File  ${NORMAL_DIRECTORY}/eicar.com

    # Different path
    Create File  ${NORMAL_DIRECTORY}/eicar2.com  ${EICAR_STRING}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar2.com
    wait_for_log_contains_from_mark  ${td_mark}  Threat ID: 49f9af79-a8bc-5436-9d3a-404a461a976e
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar2.com
    wait_for_log_contains_from_mark  ${td_mark}  Threat ID: 49f9af79-a8bc-5436-9d3a-404a461a976e
    Remove File  ${NORMAL_DIRECTORY}/eicar2.com

    # Different contents
    Create File  ${NORMAL_DIRECTORY}/eicar.com  ${EICAR_STRING} foo
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar.com
    wait_for_log_contains_from_mark  ${td_mark}  Threat ID: e692d7ef-4848-5e7d-b530-64a674a3ad0a
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar.com
    wait_for_log_contains_from_mark  ${td_mark}  Threat ID: e692d7ef-4848-5e7d-b530-64a674a3ad0a
    Remove File  ${NORMAL_DIRECTORY}/eicar.com

Threat Detector Can Bootstrap New SUSI After A Failed Initialization
    #Unload SUSI
    register cleanup    Exclude VDL Folder Missing Errors
    restart sophos_threat_detector

    Create SUSI Initialisation Error

    #Attempt to initialize SUSI, bootstrap and init will fail due to the vdl directory missing
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${AVSCANNER}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}

    #Restore vdl files
    Move Directory  ${VDL_TEMP_DESTINATION}  ${VDL_DIRECTORY}

    #Attempt to initialize SUSI, a clean result means sucessfull initialization
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${AVSCANNER}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}



Sophos Threat Detector Triggers SafeStore Rescan When SUSI Config Changes
    Remove File    ${SOPHOS_INSTALL}/plugins/av/var/disable_safestore
    # Start from known place with a CORC policy with an empty allow list
    Stop sophos_threat_detector
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Start sophos_threat_detector
    wait_for_log_contains_from_mark  ${td_mark}  MetadataRescanServer starting listening on socket

    # Wait for AV to restart TD if it is going to
    wait_for_possible_log_contains_from_mark  ${av_mark}  UnixSocket <> ProcessControlClient connected  timeout=${5}
    Sleep  ${1}

    # Send CORC policy with populated allow list to product, to trigger SafeStore rescan
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Send CORC Policy To Base  corc_policy.xml
    wait_for_log_contains_from_mark  ${td_mark}  Number of SHA256 allow-listed items: 3
    wait_for_log_contains_from_mark  ${td_mark}  Number of Path allow-listed items: 4
    wait_for_log_contains_from_mark  ${td_mark}  SUSI settings changed
    wait_for_log_contains_from_mark  ${td_mark}  Triggering rescan of SafeStore database
    wait_for_log_contains_from_mark  ${safestore_mark}  SafeStore Database Rescan request received


Sophos Threat Detector Does Not Detect Allow Listed File By Sha256
    # Start from known place with a CORC policy with an empty allow list
    Stop sophos_threat_detector
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Start sophos_threat_detector

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}

    Send CORC Policy To Base  corc_policy.xml
    wait_for_log_contains_from_mark  ${av_mark}  Added SHA256 to allow list: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    wait_for_log_contains_from_mark  ${td_mark}  Number of SHA256 allow-listed items: 3

    # Create threat to scan
    ${allow_listed_threat_file} =  Set Variable  /tmp_test/MLengHighScore.exe
    Create Directory  /tmp_test/
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${allow_listed_threat_file}
    Register Cleanup  Remove File  ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} /tmp_test/MLengHighScore.exe
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    # File is allowed and not treated as a threat
    wait_for_log_contains_from_mark  ${td_mark}  Allowed by SHA256: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9

    # File allowed so should still exist
    Should Exist  ${allow_listed_threat_file}


Sophos Threat Detector Does Not Detect Archive Allow Listed By Sha256
    ${directory_under_test} =    Set Variable    /tmp_test/a/path/
    ${allow_listed_threat_file} =    Set variable    ${directory_under_test}zipfile.zip
    ${directory_for_zipfile} =    Set Variable      /zip_test/
    ${zip_contents_file} =    Set Variable    ${directory_for_zipfile}eicar.com

    Stop sophos_threat_detector
    #Policy
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml

    #Create zip file containing ML
    Create Directory  ${directory_under_test}
    Create Directory  ${directory_for_zipfile}
    Create Temporary eicar in  ${zip_contents_file}
    zip_file   ${allow_listed_threat_file}   ${zip_contents_file}
    File Should Exist    ${allow_listed_threat_file}
    Register Cleanup  Remove Directory    ${directory_under_test}     recursive=True
    Remove Directory   ${directory_for_zipfile}    recursive=True
    Start sophos_threat_detector

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}

    # Allow-list the file
    ${archive_sha} =  Get SHA256  ${allow_listed_threat_file}
    ${allowlisted_sha} =    Create List     ${archive_sha}
    ${corc_policy} =    Create CORC Policy    whitelist_sha256s=${allowlisted_sha}
    Send CORC Policy To Base From Content    ${corc_policy}

    wait_for_log_contains_from_mark  ${av_mark}  Added SHA256 to allow list: ${archive_sha}
    wait_for_log_contains_from_mark  ${td_mark}    Number of SHA256 allow-listed items: 1

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${allow_listed_threat_file}
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    # File is allowed and not treated as a threat
    wait_for_log_contains_from_mark  ${td_mark}  Calculated the SHA256 of ${allow_listed_threat_file}: ${archive_sha}
    # A detection occurred for the ML but we are allowing the zip file still
    wait_for_log_contains_from_mark  ${td_mark}  Allowing ${allow_listed_threat_file} with ${archive_sha}

    Should Exist  ${allow_listed_threat_file}


Sophos Threat Detector Does Not Detect Allow Listed File By Path
    ${directory_under_test} =    Set Variable    /tmp_test/a/path/
    ${directory_allow_glob} =    Set Variable    /tmp_test/*/path/
    ${allow_listed_threat_file} =    Set variable    ${directory_under_test}eicar.com

    Stop sophos_threat_detector
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Start sophos_threat_detector

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}

    # Allow-list the file
    ${allowlisted_paths} =    Create List     ${directory_allow_glob}
    ${corc_policy} =    Create CORC Policy    whitelist_paths=${allowlisted_paths}
    Send CORC Policy To Base From Content    ${corc_policy}

    wait_for_log_contains_from_mark  ${av_mark}  Added path to allow list: ${directory_allow_glob}
    wait_for_log_contains_from_mark  ${td_mark}  Number of Path allow-listed items: 1

    # Create threat to scan
    Create Directory  ${directory_under_test}
    Create Temporary eicar in  ${allow_listed_threat_file}
    Register Cleanup  Remove File  ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${allow_listed_threat_file}
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    # File is allowed and not treated as a threat
    wait_for_log_contains_from_mark  ${td_mark}  Allowed by path: ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}


Sophos Threat Detector Does Not Detect Archive Allow Listed By Path
    ${directory_under_test} =    Set Variable    /tmp_test/a/path/
    ${allow_listed_threat_file} =    Set variable    ${directory_under_test}zipfile.zip
    ${directory_for_zipfile} =    Set Variable      /zip_test/
    ${zip_contents_file} =    Set Variable    ${directory_for_zipfile}eicar.com

    Stop sophos_threat_detector
    #Policy
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml

    #Create zip file containing ML
    Create Directory  ${directory_under_test}
    Create Directory  ${directory_for_zipfile}
    Create Temporary eicar in  ${zip_contents_file}
    zip_file   ${allow_listed_threat_file}   ${zip_contents_file}
    File Should Exist    ${allow_listed_threat_file}
    Register Cleanup  Remove Directory    ${directory_under_test}     recursive=True
    Remove Directory   ${directory_for_zipfile}    recursive=True
    Start sophos_threat_detector

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}

    # Allow-list the file
    ${allowlisted_paths} =    Create List     ${allow_listed_threat_file}
    ${corc_policy} =    Create CORC Policy    whitelist_paths=${allowlisted_paths}
    Send CORC Policy To Base From Content    ${corc_policy}

    wait_for_log_contains_from_mark  ${av_mark}  Added path to allow list: ${allow_listed_threat_file}
    wait_for_log_contains_from_mark  ${td_mark}  Number of Path allow-listed items: 1

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${allow_listed_threat_file}
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    # File is allowed and not treated as a threat
    wait_for_log_contains_from_mark  ${td_mark}  Allowed by path: ${allow_listed_threat_file}
    # A detection occurred for the ML but we are allowing the zip file still
    wait_for_log_contains_from_mark  ${td_mark}  Allowing ${allow_listed_threat_file} as path is in allow list

    Should Exist  ${allow_listed_threat_file}


Sophos Threat Detector Does Not Detect Long Allow List Entry
    ${directory_under_test} =    Set Variable    /tmp_test/a_path_that_has_one_hundred_characters_yeaha_thats_right_one_hundred_wholesome_characters/
    ${allow_listed_threat_file} =    Set variable    ${directory_under_test}MLengHighScore.exe

    Stop sophos_threat_detector
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Start sophos_threat_detector

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}

    # Allow-list the file
    ${allowlisted_paths} =    Create List     ${directory_under_test}
    ${corc_policy} =    Create CORC Policy    whitelist_paths=${allowlisted_paths}
    Send CORC Policy To Base From Content    ${corc_policy}

    wait_for_log_contains_from_mark  ${av_mark}  Added path to allow list: ${directory_under_test}
    wait_for_log_contains_from_mark  ${td_mark}  Number of Path allow-listed items: 1

    # Create threat to scan
    Create Directory  ${directory_under_test}
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${allow_listed_threat_file}
    Register Cleanup  Remove File  ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${allow_listed_threat_file}
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    # File is allowed and not treated as a threat
    wait_for_log_contains_from_mark  ${td_mark}  Allowed by path: ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}


Sophos Threat Detector Does Not Detect Mounted Allow List Entry
    ${basedir_under_test} =    Set Variable    /tmp_test/a/path/
    ${directory_under_test} =    Set Variable    ${basedir_under_test}mount/
    ${allow_listed_threat_file} =    Set variable    ${directory_under_test}MLengHighScore.exe

    Require Filesystem    ext4
    ${image} =    Copy And Extract Image     ext4FileSystem
    Register Cleanup  Remove Directory  ${basedir_under_test}  recursive=True
    Mount Image     ${directory_under_test}      ${image}      ext4

    Stop sophos_threat_detector
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Start sophos_threat_detector

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}

    # Allow-list the file
    ${allowlisted_paths} =    Create List     ${directory_under_test}
    ${corc_policy} =    Create CORC Policy    whitelist_paths=${allowlisted_paths}
    Send CORC Policy To Base From Content    ${corc_policy}

    wait_for_log_contains_from_mark  ${av_mark}  Added path to allow list: ${directory_under_test}
    wait_for_log_contains_from_mark  ${td_mark}  Number of Path allow-listed items: 1

    # Create threat to scan
    Create Directory  ${directory_under_test}
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${allow_listed_threat_file}
    Register Cleanup  Remove File  ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${allow_listed_threat_file}
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    # File is allowed and not treated as a threat
    wait_for_log_contains_from_mark  ${td_mark}  Allowed by path: ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}


Threat Detector Pid Lock Permissions Are Correct
    ${rc}   ${output} =    Run And Return Rc And Output  stat -c '%a' ${COMPONENT_ROOT_PATH}/chroot/var/threat_detector.pid
    Should Be Equal As Integers   ${output}  ${640}


Threat Detector Creates Metadata Rescan Socket With Correct Permissions
    ${owner} =    Get File Owner    ${METADATA_RESCAN_SOCKET}
    ${group} =    Get File Group    ${METADATA_RESCAN_SOCKET}
    Should Be Equal    ${owner}    sophos-spl-threat-detector
    Should Be Equal    ${group}    sophos-spl-group

Sophos Threat Detector Is Not Shutdown On A New Policy
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark

    Force SUSI to be initialized

    Send CORE Policy To Base  core_policy/CORE-36_ml_disabled.xml
    register cleanup  Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    Wait For Sophos Threat Detector Log Contains After Mark  Machine Learning will be disabled  ${threat_detector_mark}
    Wait For Sophos Threat Detector Log Contains After Mark  Susi configuration reloaded  ${threat_detector_mark}

    ${susi_debug_mark} =  Get SUSI Debug Log Mark
    Check avscanner can detect eicar
    Wait For SUSI Debug Log Contains After Mark    "machineLearning" : false  ${susi_debug_mark}

    ${threat_detector_mark2} =  Get Sophos Threat Detector Log Mark
    ${susi_debug_mark2} =  Get SUSI Debug Log Mark
    Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    Wait For Sophos Threat Detector Log Contains After Mark  Machine Learning will be enabled  ${threat_detector_mark2}
    Wait For Sophos Threat Detector Log Contains After Mark  Susi configuration reloaded  ${threat_detector_mark2}
    Check avscanner can detect eicar
    Wait For SUSI Debug Log Contains After Mark    "machineLearning" : true  ${susi_debug_mark2}

    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}


*** Keywords ***
Start AV Plugin and Force SUSI to be initialized
    Start AV Plugin And Threat Detector
    Wait until threat detector running
    Force SUSI to be initialized

Start sophos_threat_detector and Force SUSI to be initialized
    Start sophos_threat_detector
    Wait until threat detector running
    Force SUSI to be initialized

Create Temporary eicar in
    [Arguments]  ${path}
    Create File  ${path}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${path}

restart sophos_threat_detector
    ${INITIAL_SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Log  Initial PID: ${INITIAL_SOPHOS_THREAT_DETECTOR_PID}
    Stop Sophos_threat_detector
    Start Sophos_threat_detector
    ${END_SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Log  Restarted PID: ${END_SOPHOS_THREAT_DETECTOR_PID}
    Should Not Be Equal As Integers  ${INITIAL_SOPHOS_THREAT_DETECTOR_PID}  ${END_SOPHOS_THREAT_DETECTOR_PID}


check sophos_threat_dector log for successful global rep lookup
    [Arguments]  ${mark}
    Check Log Contains After Mark    ${SUSI_DEBUG_LOG_PATH}   [GR] Connection \#0 to host 4.sophosxl.net left intact    ${mark}

AVSophosThreatDetector Suite Setup
    Log  AVSophosThreatDetector Suite Setup
    Require Plugin Installed and Running  DEBUG

AVSophosThreatDetector Suite TearDown
    Log  AVSophosThreatDetector Suite TearDown
    Send CORE Policy To Base    core_policy/CORE-36_oa_disabled.xml

AVSophosThreatDetector Test Setup
    Require Plugin Installed and Running  DEBUG
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/disable_safestore
    register on fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register on fail  dump log  ${WATCHDOG_LOG}
    register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/wdctl.log
    register on fail  dump log  ${SOPHOS_INSTALL}/plugins/av/log/av.log
    register on fail  dump log   ${SUSI_DEBUG_LOG_PATH}
    register on fail  dump threads  ${SOPHOS_THREAT_DETECTOR_BINARY}
    register on fail  dump threads  ${PLUGIN_BINARY}

    Register Cleanup      Check All Product Logs Do Not Contain Error
    Register Cleanup      Exclude MCS Router is dead
    Register Cleanup      Exclude CustomerID Failed To Read Error
    Register Cleanup      Require No Unhandled Exception
    Register Cleanup      Check For Coredumps  ${TEST NAME}
    Register Cleanup      Check Dmesg For Segfaults
    Register Cleanup      Cleanup Safestore Database

Create Default Machine Id
    Create File  ${MACHINEID_FILE}  3ccfaf097584e65c6c725c6827e186bb


AVSophosThreatDetector Test TearDown
    # restore machineID file - needs to be before cleanup functions
    # since cleanup functions can restart threat detector
    Create Default Machine Id

    #run teardown functions
    run cleanup functions
    run failure functions if failed

    Remove File  ${CUSTOMERID_FILE}

    run keyword if test failed  Restart AV Plugin And Clear The Logs For Integration Tests

Alter Hosts
    ## Back up /etc/hosts
    ## Register cleanup function
    alter etc hosts
    register cleanup  Restore hosts

Restore hosts
    restore etc hosts
