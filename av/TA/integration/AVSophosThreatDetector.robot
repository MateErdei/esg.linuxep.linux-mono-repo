*** Settings ***
Documentation    Product tests of sophos_threat_detector
Force Tags       INTEGRATION  SOPHOS_THREAT_DETECTOR

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot

Library         ../Libs/CoreDumps.py
Library         ../Libs/OnFail.py
Library         ../Libs/ProcessUtils.py

Suite Setup     AVSophosThreatDetector Suite Setup
Suite Teardown  AVSophosThreatDetector Suite TearDown

Test Setup      AVSophosThreatDetector Test Setup
Test Teardown   AVSophosThreatDetector Test TearDown

*** Variables ***
${CLEAN_STRING}     not an eicar
${NORMAL_DIRECTORY}     /home/vagrant/this/is/a/directory/for/scanning
${CUSTOMERID_FILE}  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/var/customer_id.txt
${MACHINEID_CHROOT_FILE}  ${COMPONENT_ROOT_PATH}/chroot${SOPHOS_INSTALL}/base/etc/machine_id.txt
${MACHINEID_FILE}   ${SOPHOS_INSTALL}/base/etc/machine_id.txt


*** Test Cases ***
Test Global Rep works in chroot
    Restart sophos_threat_detector and mark logs
    scan GR test file
    check sophos_threat_dector log for successful global rep lookup

Sophos Threat Detector Has No Unnecessary Capabilities
    # ensure that threat_detector is fully started up
    Restart sophos_threat_detector and mark logs

    ${rc}   ${pid} =       Run And Return Rc And Output    pgrep sophos_threat
    ${rc}   ${output} =    Run And Return Rc And Output    getpcaps ${pid}
    Should Not Contain  ${output}  cap_sys_chroot
    # Handle different format of the output from getpcaps on Ubuntu 20.04
    IF  ("${output}" != "Capabilities for `${pid}\': =") and ("${output}" != "${pid}: =")
        Fail  msg=Unexpected capabilities: ${output}
    END


Threat detector does not recreate logging symlink if present
    Should Exist   ${CHROOT_LOGGING_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}/sophos_threat_detector.log
    Restart sophos_threat_detector and mark logs
    Threat Detector Log Should Not Contain With Offset   LogSetup <> Create symlink for logs at
    Threat Detector Log Should Not Contain With Offset   LogSetup <> Failed to create symlink for logs at

Threat detector recreates logging symlink if missing
    register cleanup   Exclude STD Symlink Error
    register cleanup   Install With Base SDDS
    Should Exist   ${CHROOT_LOGGING_SYMLINK}

    Run Process   rm   ${CHROOT_LOGGING_SYMLINK}
    Should Not Exist   ${CHROOT_LOGGING_SYMLINK}
    Restart sophos_threat_detector and mark logs

    Wait Until Sophos Threat Detector Log Contains With Offset  LogSetup <> Create symlink for logs at
    Threat Detector Log Should Not Contain With Offset   LogSetup <> Failed to create symlink for logs at
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
    Restart sophos_threat_detector and mark logs

    Sophos Threat Detector Log Contains With Offset   LogSetup <> Failed to create symlink for logs at
    Threat Detector Log Should Not Contain With Offset   LogSetup <> Create symlink for logs at
    Should Not Exist   ${CHROOT_LOGGING_SYMLINK}

Threat Detector Restarts When /etc/hosts changed
    # ensure that both AV and threat_detector are ready
    Wait Until AV Plugin Log Contains  Starting sophos_threat_detector monitor
    Wait Until Sophos Threat Detector Log Contains  Process Controller Server starting listening on socket: /var/process_control_socket  timeout=120

    ${SOPHOS_THREAT_DETECTOR_PID_AT_START} =  Get Sophos Threat Detector PID From File

    Mark AV Log
    Mark Sophos Threat Detector Log

    Alter Hosts

    # wait for AV log
    Wait Until AV Plugin Log Contains With Offset  Restarting sophos_threat_detector as the system configuration has changed
    Wait Until Sophos Threat Detector Shutdown File Exists
    Mark Sophos Threat Detector Log
    Wait Until Sophos Threat Detector Log Contains With Offset  Process Controller Server starting listening on socket: /var/process_control_socket  timeout=120

    Wait until threat detector running with offset
    ${SOPHOS_THREAT_DETECTOR_PID_AT_END} =  Get Sophos Threat Detector PID From File
    Should Not Be Equal As Integers  ${SOPHOS_THREAT_DETECTOR_PID_AT_START}  ${SOPHOS_THREAT_DETECTOR_PID_AT_END}

Threat Detector restarts if no scans requested within the configured timeout
    Stop sophos_threat_detector
    Create File  ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config  {"shutdownTimeout":15}
    Register Cleanup   Remove File   ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config
    Register Cleanup   Stop sophos_threat_detector

    Mark Sophos Threat Detector Log
    Start sophos_threat_detector
    Wait Until Sophos Threat Detector Log Contains With Offset  Process Controller Server starting listening on socket: /var/process_control_socket  timeout=120
    Wait Until Sophos Threat Detector Log Contains With Offset  Default shutdown timeout set to 15 seconds.
    Wait Until Sophos Threat Detector Log Contains With Offset  Setting shutdown timeout to

    Mark Sophos Threat Detector Log
    # Request a scan in order to load SUSI
    Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${SOPHOS_THREAT_DETECTOR_PID_AT_START} =  Get Sophos Threat Detector PID From File
    Wait Until Sophos Threat Detector Log Contains With Offset  Default shutdown timeout set to 15 seconds.
    Wait Until Sophos Threat Detector Log Contains With Offset  Setting shutdown timeout to

    # No scans requested for ${timeout} seconds - shutting down.
    Wait Until Sophos Threat Detector Log Contains With Offset  No scans requested  timeout=20
    Wait Until Sophos Threat Detector Shutdown File Exists
    Wait Until Sophos Threat Detector Log Contains With Offset  Sophos Threat Detector is exiting

    Wait Until Sophos Threat Detector Log Contains With Offset  Process Controller Server starting listening on socket: /var/process_control_socket  timeout=120

    Wait until threat detector running with offset
    ${SOPHOS_THREAT_DETECTOR_PID_AT_END} =  Get Sophos Threat Detector PID From File
    Should Not Be Equal As Integers  ${SOPHOS_THREAT_DETECTOR_PID_AT_START}  ${SOPHOS_THREAT_DETECTOR_PID_AT_END}

Threat Detector prolongs timeout if a scan is requested within the configured timeout
    Stop sophos_threat_detector
    Create File  ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config  {"shutdownTimeout":15}
    Register Cleanup   Remove File   ${AV_PLUGIN_PATH}/chroot/etc/threat_detector_config
    Register Cleanup   Stop sophos_threat_detector

    Mark Sophos Threat Detector Log
    Start sophos_threat_detector
    Wait Until Sophos Threat Detector Log Contains With Offset  Process Controller Server starting listening on socket: /var/process_control_socket  timeout=120
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Wait Until Sophos Threat Detector Log Contains With Offset  Default shutdown timeout set to 15 seconds.
    Wait Until Sophos Threat Detector Log Contains With Offset  Setting shutdown timeout to

    Mark Sophos Threat Detector Log
    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    # Scan requested less than ${timeout1} seconds ago - continuing
    Wait Until Sophos Threat Detector Log Contains With Offset  Scan requested less than
    Wait Until Sophos Threat Detector Log Contains With Offset  Setting shutdown timeout to

    # No scans requested for ${timeout2} seconds - shutting down.
    Wait Until Sophos Threat Detector Log Contains With Offset  No scans requested for  timeout=20
    Wait Until Sophos Threat Detector Log Contains With Offset  Sophos Threat Detector is exiting
    Wait Until Sophos Threat Detector Log Contains With Offset  Process Controller Server starting listening on socket: /var/process_control_socket  timeout=120
    Check Sophos Threat Detector has different PID  ${SOPHOS_THREAT_DETECTOR_PID}

    Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace

Threat Detector Is Given Non-Permission EndpointId
    [Tags]  FAULT INJECTION
    register cleanup  Exclude MachineID Permission Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}  ab7b6758a3ab11ba8a51d25aa06d1cf4
    Run Process  chmod  000  ${MACHINEID_FILE}
    Register Cleanup  Remove File  ${MACHINEID_FILE}
    Register Cleanup    Stop Sophos_threat_detector
    Remove File  ${MACHINEID_CHROOT_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  Failed to copy: "${MACHINEID_FILE}"
    Sophos Threat Detector Log Contains With Offset  Failed to read machine ID - using default value

SUSI Is Given Non-Permission EndpointId
    [Tags]  FAULT INJECTION
    register cleanup  Exclude MachineID Permission Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}  ab7b6758a3ab11ba8a51d25aa06d1cf4
    Register Cleanup  Remove File  ${MACHINEID_FILE}
    Start AV Plugin
    Wait until threat detector running with offset
    Run Process  chmod  000  ${MACHINEID_CHROOT_FILE}
    Register Cleanup    Remove File  ${MACHINEID_CHROOT_FILE}
    Register Cleanup    Stop Sophos_threat_detector
    Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  Failed to read machine ID - using default value

SUSI Is Given Empty EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude EndpointID Cannot Be Empty
    register cleanup    Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  EndpointID cannot be empty


SUSI Is Given A New Line As EndpointId
    [Tags]  FAULT INJECTION
    register cleanup     Exclude MachineID New Line Error
    register cleanup     Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}  \n
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  EndpointID cannot contain a new line

SUSI Is Given An Empty Space As EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Empty Space Error
    register cleanup    Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}  ${SPACE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  EndpointID cannot contain a empty space

SUSI Is Given Short EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Shoulb Be 32 Hex Error
    register cleanup    Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}  d22829d94b76c016ec4e04b08baef
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  EndpointID should be 32 hex characters

SUSI Is Given Long EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Shoulb Be 32 Hex Error
    register cleanup    Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}  d22829d94b76c016ec4e04b08baefaaaaaaaaaaaaaaa
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  EndpointID should be 32 hex characters

SUSI Is Given Non-hex EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Hex Format Error
    register cleanup    Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${MACHINEID_FILE}  GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  EndpointID must be in hex format

SUSI Is Given Non-UTF As EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Hex Format Error
    register cleanup    Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
    ${rc}   ${output} =    Run And Return Rc And Output  echo -n "ソフォスソフォスソフォスソフォス" | iconv -f utf-8 -t euc-jp > ${MACHINEID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  EndpointID must be in hex format

SUSI Is Given Binary EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Failed To Read Error
    register cleanup    Exclude MachineID New Line Error
    Stop sophos_threat_detector and mark log
    Copy File  /bin/true  ${MACHINEID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  Failed to read machine ID - using default value

SUSI Is Given Large File EndpointId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude MachineID Shoulb Be 32 Hex Error
    register cleanup    Exclude MachineID Failed To Read Error
    Stop sophos_threat_detector and mark log
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
    Sophos Threat Detector Log Contains With Offset  EndpointID should be 32 hex characters

SUSI Is Given Empty CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Cannot Be Empty Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${CUSTOMERID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  CustomerID cannot be empty

SUSI Is Given A New Line As CustomerId
    [Tags]  FAULT INJECTION
    register cleanup   Exclude CustomerID New Line Error
    register cleanup   Exclude CustomerID Hex Format Error
    register cleanup   Exclude CustomerID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${CUSTOMERID_FILE}  \n
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  CustomerID cannot contain a new line

SUSI Is Given An Empty Space As CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Cannot Be Empty Space Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${CUSTOMERID_FILE}  ${SPACE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  CustomerID cannot contain a empty space

SUSI Is Given Short CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    Stop sophos_threat_detector and mark log
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baef
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  CustomerID should be 32 hex characters

SUSI Is Given Long CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baefaaaaaaaaaaaaaaa
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  CustomerID should be 32 hex characters

SUSI Is Given Non-hex CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Create File  ${CUSTOMERID_FILE}  GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  CustomerID must be in hex format

SUSI Is Given Non-UTF As CustomerId
    [Tags]  FAULT INJECTION
    register cleanup   Exclude CustomerID Hex Format Error
    register cleanup   Exclude CustomerID Hex Format Error
    register cleanup   Exclude CustomerID Failed To Read Error
    Stop sophos_threat_detector and mark log
    ${rc}   ${output} =    Run And Return Rc And Output  echo -n "ソフォスソフォスソフォスソフォス" | iconv -f utf-8 -t euc-jp > ${CUSTOMERID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  CustomerID must be in hex format

SUSI Is Given Binary CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID New Line Error
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    Stop sophos_threat_detector and mark log
    Copy File  /bin/true  ${CUSTOMERID_FILE}
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  Failed to read customerID - using default value

SUSI Is Given Large File CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Hex Format Error
    register cleanup    Exclude CustomerID Failed To Read Error
    register cleanup    Exclude CustomerID Shoulb Be 32 Hex Error
    Stop sophos_threat_detector and mark log
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
    Sophos Threat Detector Log Contains With Offset  CustomerID should be 32 hex characters

SUSI Is Given Non-Permission CustomerId
    [Tags]  FAULT INJECTION
    register cleanup    Exclude CustomerID Failed To Read Error
    register cleanup    Exclude CustomerID Hex Format Error
    Stop sophos_threat_detector and mark log
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baeffaa
    Run Process  chmod  000  ${CUSTOMERID_FILE}
    Register Cleanup    Remove File  ${CUSTOMERID_FILE}
    Register Cleanup    Stop Sophos_threat_detector
    Start sophos_threat_detector and Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  Failed to read customerID - using default value

Threat Detector Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    register cleanup    Exclude Watchdog Log Unable To Open File Error
    Create Temporary eicar in  ${NORMAL_DIRECTORY}/naughty_eicar

    Mark Sophos Threat Detector Log
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naughty_eicar'"
    Sophos Threat Detector Log Contains With Offset  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/naughty_eicar

    Run  chmod 444 ${THREAT_DETECTOR_LOG_PATH}
    Register Cleanup  Run  chmod 600 ${THREAT_DETECTOR_LOG_PATH}
    Register Cleanup  Stop Sophos_threat_detector

    # use the basic restart method, as threat detector will not be logging
    restart sophos_threat_detector

    ${result} =  Run Process  ls  -l  ${THREAT_DETECTOR_LOG_PATH}
    Log  New permissions: ${result.stdout}

    Mark AV Log
    Mark Sophos Threat Detector Log

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar

    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naughty_eicar'"
    Threat Detector Log Should Not Contain With Offset  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/naughty_eicar

SUSI Can Work Despite Specified Log File Being Read-Only
    [Tags]  FAULT INJECTION
    register cleanup    Exclude Watchdog Log Unable To Open File Error
    Create Temporary eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    register on fail  dump log  ${AV_LOG_PATH}

    Mark Susi Debug Log

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naughty_eicar'"
    SUSI Debug Log Contains With Offset  SUSI scan for ${NORMAL_DIRECTORY}/naughty_eicar

    Run  chmod 444 ${SUSI_DEBUG_LOG_PATH}
    Register Cleanup  Stop Sophos_threat_detector
    Register Cleanup  Run  chmod 600 ${SUSI_DEBUG_LOG_PATH}

    restart sophos_threat_detector and mark logs

    ${result} =  Run Process  ls  -l  ${SUSI_DEBUG_LOG_PATH}
    Log  New permissions: ${result.stdout}

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/naughty_eicar
    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naughty_eicar'"
    SUSI Debug Log Does Not Contain With Offset  SUSI scan for ${NORMAL_DIRECTORY}/naughty_eicar

SUSI Debug Log Does Not Contain Info Level Logs By Default
    register cleanup     Exclude Watchdog Log Unable To Open File Error
    register cleanup     Stop sophos_threat_detector
    register cleanup     Set Log Level  DEBUG

    Create Temporary eicar in  ${NORMAL_DIRECTORY}/eicar.com
    Set Log Level  INFO

    restart sophos_threat_detector and mark logs

    # Request a scan in order to load SUSI
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /bin/bash
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Wait Until File exists  ${SUSI_DEBUG_LOG_PATH}
    Mark Susi Debug Log
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar.com
    Wait Until Sophos Threat Detector Log Contains With Offset  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/eicar.com

    # Confirm that no info-level SUSI log messages were printed (ie. those starting with "I")
    ${contents}  Get File Contents From Offset   ${SUSI_DEBUG_LOG_PATH}   ${SUSI_DEBUG_LOG_MARK}
    @{lines}=  Split to lines  ${contents}
    FOR  ${line}  IN  @{lines}
        Log  ${line}
        Should Not Start With  ${line}  I
    END

Sophos Threat Detector Is Not Shutdown On A New Policy
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    Force SUSI to be initialized

    # restart AV to force policy to be applied & sent to threat detector
    Stop AV Plugin Process
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now_Lookup_Disabled.xml
    Start AV Plugin Process
    Wait Until Sophos Threat Detector Log Contains With Offset  Susi configuration reloaded
    mark susi debug log
    Check avscanner can detect eicar
    Wait Until SUSI DEBUG Log Contains With Offset    "enableLookup" : false

    mark sophos threat detector log
    mark susi debug log
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now.xml
    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be enabled
    Wait Until Sophos Threat Detector Log Contains With Offset  Susi configuration reloaded
    Check avscanner can detect eicar
    Wait Until SUSI DEBUG Log Contains With Offset    "enableLookup" : true

    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

Sophos Threat Detector Is Ignoring Reload Request
    #unload susi
    Stop sophos_threat_detector
    Mark Sophos Threat Detector Log
    Start sophos_threat_detector

    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Stop AV Plugin Process
    Register Cleanup   Send Sav Policy With No Scheduled Scans
    Send Sav Policy With Imminent Scheduled Scan To Base
    Start AV Plugin Process

    Wait Until Sophos Threat Detector Log Contains With Offset  Skipping susi reload because susi is not initialised
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

*** Keywords ***
Stop sophos_threat_detector and mark log
    Stop sophos_threat_detector
    Mark Sophos Threat Detector Log

Start AV Plugin and Force SUSI to be initialized
    Start AV Plugin
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
    Susi Debug Log Contains  =GR= Connection \#0 to host 4.sophosxl.net left intact

AVSophosThreatDetector Suite Setup
    Log  AVSophosThreatDetector Suite Setup
    Require Plugin Installed and Running  DEBUG
    # TODO: Remove stopping of soapd once file descriptor usage issue is fixed
    Stop soapd

AVSophosThreatDetector Suite TearDown
    Log  AVSophosThreatDetector Suite TearDown

AVSophosThreatDetector Test Setup
    Require Plugin Installed and Running  DEBUG
    Run Keyword and Ignore Error   Run Shell Process   ${SOPHOS_INSTALL}/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

    Mark AV Log
    Mark Sophos Threat Detector Log

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

AVSophosThreatDetector Test TearDown
    #run teardown functions
    run cleanup functions
    run failure functions if failed

    #restore machineID file
    Create File  ${MACHINEID_FILE}  3ccfaf097584e65c6c725c6827e186bb
    Remove File  ${CUSTOMERID_FILE}

    run keyword if test failed  Restart AV Plugin And Clear The Logs For Integration Tests

Alter Hosts
    ## Back up /etc/hosts
    ## Register cleanup function
    alter etc hosts
    register cleanup  Restore hosts

Restore hosts
    restore etc hosts

Get Shutdown Timeout From Threat Detector Log
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    ${contents} =  Get File Contents From Offset  ${THREAT_DETECTOR_LOG_PATH}  ${offset}
    ${lines} =  Get Lines Containing String  ${contents}  Setting shutdown timeout to
    ${split_line} =  Split String From Right  ${lines}
    ${timeout} =  Strip String  ${split_line}[${10}]
    [return]  ${timeout}