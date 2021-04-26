*** Settings ***
Documentation    Product tests of sophos_threat_detector
Force Tags       INTEGRATION  SOPHOS_THREAT_DETECTOR

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Library         ../Libs/OnFail.py

Suite Setup     AVSophosThreatDetector Suite Setup
Suite Teardown  AVSophosThreatDetector Suite TearDown

Test Setup      AVSophosThreatDetector Test Setup
Test Teardown   AVSophosThreatDetector Test TearDown

*** Variables ***
${CLI_SCANNER_PATH}  ${COMPONENT_ROOT_PATH}/bin/avscanner

*** Test Cases ***
Test Global Rep works in chroot
    run on failure  dump log   ${SUSI_DEBUG_LOG_PATH}
    run on failure  dump log   ${THREAT_DETECTOR_LOG_PATH}
    set sophos_threat_detector log level
    Restart sophos_threat_detector
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
    Restart sophos_threat_detector
    Threat Detector Does Not Log Contain   LogSetup <> Create symlink for logs at
    Threat Detector Does Not Log Contain   LogSetup <> Failed to create symlink for logs at

Threat detector recreates logging symlink if missing
    register cleanup   Install With Base SDDS
    Should Exist   ${THREAT_DETECTOR_LOG_SYMLINK}
    Should Exist   ${CHROOT_LOGGING_SYMLINK}

    Run Process   rm   ${CHROOT_LOGGING_SYMLINK}
    Should Not Exist   ${CHROOT_LOGGING_SYMLINK}
    Create File   ${THREAT_DETECTOR_LOG_PATH}   # truncate the log
    Restart sophos_threat_detector

    Threat Detector Log Contains   LogSetup <> Create symlink for logs at
    Threat Detector Does Not Log Contain   LogSetup <> Failed to create symlink for logs at
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
    Create File   ${THREAT_DETECTOR_LOG_PATH}   # truncate the log
    Restart sophos_threat_detector

    Threat Detector Log Contains   LogSetup <> Failed to create symlink for logs at
    Threat Detector Does Not Log Contain   LogSetup <> Create symlink for logs at
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


*** Keywords ***

set sophos_threat_detector log level
    Set Log Level  DEBUG

Restart sophos_threat_detector
    Stop sophos_threat_detector
    Start sophos_threat_detector
    Mark AV Log
    wait for sophos_threat_detector to be running

wait for sophos_threat_detector to be running
    Wait until threat detector running

Kill sophos_threat_detector
   ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
   Run Process   /bin/kill   -9   ${output}

Stop sophos_threat_detector
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   threat_detector
    Should Be Equal As Integers    ${result.rc}    0

Start sophos_threat_detector
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start  threat_detector
    Should Be Equal As Integers    ${result.rc}    0

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
    run teardown functions

Alter Hosts
    ## Back up /etc/hosts
    ## Register cleanup function
    alter etc hosts
    register cleanup  Restore hosts

Restore hosts
    restore etc hosts
