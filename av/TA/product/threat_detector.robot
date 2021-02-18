*** Settings ***
Documentation   Product tests for sophos_threat_detector
Force Tags      PRODUCT

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/OnFail.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Test Teardown  Threat Detector Test Teardown

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log
${TESTTMP}  /tmp_test/SSPLAVTests
${SOPHOS_THREAT_DETECTOR_BINARY_LAUNCHER}  ${SOPHOS_THREAT_DETECTOR_BINARY}_launcher

*** Keywords ***

List AV Plugin Path
    Create Directory  ${TESTTMP}
    ${result} =  Run Process  ls  -lR  ${AV_PLUGIN_PATH}  stdout=${TESTTMP}/lsstdout  stderr=STDOUT
    Log  ls -lR: ${result.stdout}
    Remove File  ${TESTTMP}/lsstdout

Threat Detector Test Teardown
    List AV Plugin Path
    run teardown functions
    Component Test TearDown

Start AV
    Remove Files   /tmp/threat_detector.stdout  /tmp/threat_detector.stderr
    ${handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}   stdout=/tmp/threat_detector.stdout  stderr=/tmp/threat_detector.stderr
    Set Test Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${handle}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed
    # Check AV Plugin Installed checks sophos_threat_detector is started

Stop AV
    ${result} =  Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}

Verify threat detector log rotated
    List Directory  ${AV_PLUGIN_PATH}/log/sophos_threat_detector
    Should Exist  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/sophos_threat_detector.log.1

Restore hosts
    restore etc hosts

Alter Hosts
    ## Back up /etc/hosts
    ## Register cleanup function
    alter etc hosts
    register cleanup  Restore hosts


*** Test Cases ***

Threat Detector Log Rotates
    Register Cleanup   Empty Directory   ${AV_PLUGIN_PATH}/log/sophos_threat_detector/

    # Ensure the log is created
    Start AV
    Stop AV
    Increase Threat Detector Log To Max Size
    Start AV
    Wait Until Created   ${AV_PLUGIN_PATH}/log/sophos_threat_detector/sophos_threat_detector.log.1   timeout=10s
    Wait Until File Log Contains
    ...   Threat Detector Log Contains
    ...   ThreatScanner
    Stop AV

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/
    Log  ${result.stdout}

    Verify threat detector log rotated

Threat Detector Log Rotates while in chroot
    Register Cleanup   Empty Directory   ${AV_PLUGIN_PATH}/log/sophos_threat_detector/
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
    Wait Until File Log Contains
    ...   Threat Detector Log Contains
    ...   Starting listening on socket
    Stop AV

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/
    Log  ${result.stdout}

    Verify threat detector log rotated

Threat Detector Restarts When /etc/hosts changed
    Register On Fail  dump log  ${AV_LOG_PATH}
    Start AV
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    Mark AV Log
    alter hosts

    # wait for AV log
    Wait Until AV Plugin Log Contains With Offset  Restarting sophos_threat_detector as the system configuration has changed
    Wait Until AV Plugin Log Contains With Offset  Starting "${SOPHOS_THREAT_DETECTOR_BINARY_LAUNCHER}"

    Wait until threat detector running
    Check Sophos Threat Detector has different PID  ${SOPHOS_THREAT_DETECTOR_PID}


Threat detector is killed gracefully
    Start AV

    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -SIGTERM   ${pid}

    Wait Until Sophos Threat Detector Log Contains  Sophos Threat Detector received SIGTERM
    Wait Until Sophos Threat Detector Log Contains  Sophos Threat Detector is exiting because it received signal SIGTERM
    Wait Until Sophos Threat Detector Log Contains  Exiting Global Susi result =0
    Wait Until AV Plugin Log Contains  Exiting sophos_threat_detector with code: 15 E_SIGTERM
    Wait Until AV Plugin Log Contains  Starting "/opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher"