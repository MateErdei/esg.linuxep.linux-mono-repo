*** Settings ***
Documentation    Product tests of sophos_threat_detector
Force Tags       INTEGRATION  SOPHOS_THREAT_DETECTOR

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot

Library         ../Libs/OnFail.py

Suite Setup     AVSophosThreatDetector Suite Setup
Suite Teardown  AVSophosThreatDetector Suite TearDown

Test Setup      AVSophosThreatDetector Test Setup
Test Teardown   AVSophosThreatDetector Test TearDown

*** Variables ***
${CLEAN_STRING}     not an eicar
${NORMAL_DIRECTORY}     /home/vagrant/this/is/a/directory/for/scanning
${CUSTOMERID_FILE}  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/var/customer_id.txt
${MACHINEID_FILE}   ${SOPHOS_INSTALL}/base/etc/machine_id.txt


*** Test Cases ***
Test Global Rep works in chroot
    run on failure  dump log   ${SUSI_DEBUG_LOG_PATH}
    run on failure  dump log   ${THREAT_DETECTOR_LOG_PATH}
    set sophos_threat_detector log level
    Restart sophos_threat_detector and mark logs
    scan GR test file
    check sophos_threat_dector log for successful global rep lookup

Sophos Threat Detector Has No Unnecessary Capabilities
    ${rc}   ${pid} =       Run And Return Rc And Output    pgrep sophos_threat
    ${rc}   ${output} =    Run And Return Rc And Output    getpcaps ${pid}
    Should Not Contain  ${output}  cap_sys_chroot
    # Handle different format of the output from getpcaps on Ubuntu 20.04
    Run Keyword Unless  "${output}" == "Capabilities for `${pid}\': =" or "${output}" == "${pid}: ="  FAIL  msg=Enexpected capabilities: ${output}

Threat detector does not recreate logging symlink if present
    Should Exist   ${THREAT_DETECTOR_LOG_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}/sophos_threat_detector.log
    Restart sophos_threat_detector and mark logs
    Threat Detector Log Should Not Contain With Offset   LogSetup <> Create symlink for logs at
    Threat Detector Log Should Not Contain With Offset   LogSetup <> Failed to create symlink for logs at

Threat detector recreates logging symlink if missing
    register cleanup   Install With Base SDDS
    Should Exist   ${THREAT_DETECTOR_LOG_SYMLINK}
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
    register cleanup   Install With Base SDDS
    Should Exist   ${THREAT_DETECTOR_LOG_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}

    Run Process   rm   ${CHROOT_LOGGING_SYMLINK}
    # stop sophos_threat_detector from creating the link, by denying group access to the directory
    Run Process   chmod   g-rwx    ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log
    Restart sophos_threat_detector and mark logs

    Sophos Threat Detector Log Contains With Offset   LogSetup <> Failed to create symlink for logs at
    Threat Detector Log Should Not Contain With Offset   LogSetup <> Create symlink for logs at
    Should Not Exist   ${CHROOT_LOGGING_SYMLINK}


Threat Detector Restarts When /etc/hosts changed
    register cleanup   Run Keyword And Ignore Error  Log File   ${AV_LOG_PATH}  encoding_errors=replace
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Mark AV Log
    Mark Sophos Threat Detector Log
    Wait Until AV Plugin Log Contains With Offset  Starting sophos_threat_detector monitor
    Wait Until Sophos Threat Detector Log Contains  Starting listening on socket: /var/process_control_socket  timeout=120
    Alter Hosts

    # wait for AV log
    Wait Until AV Plugin Log Contains With Offset  Restarting sophos_threat_detector as the system configuration has changed
    Mark Sophos Threat Detector Log
    Wait Until Sophos Threat Detector Log Contains With Offset  Starting listening on socket: /var/process_control_socket  timeout=120

    Wait until threat detector running
    Check Sophos Threat Detector has different PID  ${SOPHOS_THREAT_DETECTOR_PID}


SUSI Is Given Empty EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${MACHINEID_FILE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID cannot be empty


SUSI Is Given A New Line As EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${MACHINEID_FILE}  \n
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID cannot contain a new line


SUSI Is Given An Empty Space As EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${MACHINEID_FILE}  ${SPACE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID cannot contain a empty space


SUSI Is Given Short EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${MACHINEID_FILE}  d22829d94b76c016ec4e04b08baef
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID should be 32 hex characters


SUSI Is Given Long EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${MACHINEID_FILE}  d22829d94b76c016ec4e04b08baefaaaaaaaaaaaaaaa
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID should be 32 hex characters


SUSI Is Given Non-hex EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${MACHINEID_FILE}  GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID must be in hex format


SUSI Is Given Non-UTF As EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    ${rc}   ${output} =    Run And Return Rc And Output  echo -n "ソフォスソフォスソフォスソフォス" | iconv -f utf-8 -t euc-jp > ${MACHINEID_FILE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID must be in hex format


SUSI Is Given Binary EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Copy File  /bin/true  ${MACHINEID_FILE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  Failed to read machine ID - using default value


SUSI Is Given Large File EndpointId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${MACHINEID_FILE}

    FOR  ${item}  IN RANGE  1  10
        Append To File  ${MACHINEID_FILE}  ab7b6758a3ab11ba8a51d25aa06d1cf4
    END

    ${idfile} =    Get File  ${MACHINEID_FILE}
    @{list} =    Split to lines  ${idfile}

    FOR  ${line}  IN  @{list}
       Log  ${line}
    END

    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  EndpointID should be 32 hex characters


SUSI Is Given Empty CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID cannot be empty


SUSI Is Given A New Line As CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}  \n
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID cannot contain a new line


SUSI Is Given An Empty Space As CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}  ${SPACE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID cannot contain a empty space


SUSI Is Given Short CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baef
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID should be 32 hex characters


SUSI Is Given Long CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baefaaaaaaaaaaaaaaa
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID should be 32 hex characters


SUSI Is Given Non-hex CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}  GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID must be in hex format


SUSI Is Given Non-UTF As CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    ${rc}   ${output} =    Run And Return Rc And Output  echo -n "ソフォスソフォスソフォスソフォス" | iconv -f utf-8 -t euc-jp > ${CUSTOMERID_FILE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID must be in hex format


SUSI Is Given Binary CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Copy File  /bin/true  ${CUSTOMERID_FILE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  Failed to read customerID - using default value


SUSI Is Given Large File CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}

    FOR  ${item}  IN RANGE  1  10
        Append To File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baeffaa
    END

    ${idfile} =    Get File  ${CUSTOMERID_FILE}
    @{list} =    Split to lines  ${idfile}

    FOR  ${line}  IN  @{list}
       Log  ${line}
    END

    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  CustomerID should be 32 hex characters


SUSI Is Given Non-Permission CustomerId
    Mark Sophos Threat Detector Log
    Stop AV Plugin
    Create File  ${CUSTOMERID_FILE}  d22829d94b76c016ec4e04b08baeffaa
    Run Process  chmod  000  ${CUSTOMERID_FILE}
    Start AV Plugin

    Wait until threat detector running

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/

    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset  Failed to read customerID - using default value


*** Keywords ***

set sophos_threat_detector log level
    Set Log Level  DEBUG

scan GR test file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${RESOURCES_PATH}/file_samples/gui.exe
    run keyword if  ${rc} != ${0}  Log  ${output}
    BuiltIn.Should Be Equal As Integers  ${rc}  ${0}  Failed to scan gui.exe

check sophos_threat_dector log for successful global rep lookup
    Susi Debug Log Contains  =GR= Connection \#0 to host 4.sophosxl.net left intact

AVSophosThreatDetector Suite Setup
    Log  AVSophosThreatDetector Suite Setup
    Install With Base SDDS

AVSophosThreatDetector Suite TearDown
    Log  AVSophosThreatDetector Suite TearDown

AVSophosThreatDetector Test Setup
    Log  AVSophosThreatDetector Test Setup
    Check AV Plugin Installed With Base
    Mark AV Log

AVSophosThreatDetector Test TearDown
    Log  AVSophosThreatDetector Test TearDown
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/logs/base/wdctl.log  encoding_errors=replace
    run teardown functions

Alter Hosts
    ## Back up /etc/hosts
    ## Register cleanup function
    alter etc hosts
    register cleanup  Restore hosts

Restore hosts
    restore etc hosts
