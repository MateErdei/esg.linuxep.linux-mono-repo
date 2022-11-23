
*** Settings ***
Documentation   Component tests for SophosThreatDetector
Force Tags      COMPONENT  PRODUCT  THREAT_DETECTOR

Library    OperatingSystem
Library    Process
Library    ../Libs/LogUtils.py
Library    ../Libs/OnFail.py

Resource    ../shared/AVResources.robot
Resource    ../shared/GlobalSetup.robot

*** Test Cases ***

Threat Detector Handles inaccessible var
    Make chroot var inaccessible
    ${mark} =  get_sophos_threat_detector_log_mark
    Run Sophos Threat Detector as sophos-spl-threat-detector without check
    wait_for_log_contains_from_mark  ${mark}
    ...  SophosThreatDetectorImpl <> ThreatDetectorMain, Exception caught at top level: filesystem error: cannot remove: Permission denied [${AV_CHROOT_VAR_DIR}/threat_detector_expected_shutdown]
    Dump Log  ${THREAT_DETECTOR_LOG_PATH}

Threat Detector Handles nonexistent var
    Remove chroot var
    ${mark} =  get_sophos_threat_detector_log_mark
    Run Sophos Threat Detector as sophos-spl-threat-detector without check
    wait_for_log_contains_from_mark  ${mark}
    ...  ThreatDetectorMain, Exception caught at top level: Unable to open lock file ${AV_CHROOT_VAR_DIR}/threat_detector.pid because No such file or directory(2)
    Dump Log  ${THREAT_DETECTOR_LOG_PATH}

*** Variables ***

*** Keywords ***

Create chroot var
    Create Directory  ${AV_CHROOT_VAR_DIR}
    Run Process  chmod  771  ${AV_CHROOT_VAR_DIR}
    Run Process  chown  root:sophos-spl-group  ${AV_CHROOT_VAR_DIR}

Remove chroot var
    Remove Directory  ${AV_CHROOT_VAR_DIR}  recursive=True
    Register Cleanup  Create chroot var

Make chroot var inaccessible
    ${result} =  Run Process  chmod  000  ${AV_CHROOT_VAR_DIR}
    ${result} =  Run Process  chown  root:root  ${AV_CHROOT_VAR_DIR}
    Register cleanup  Create chroot var

Run Sophos Threat Detector as sophos-spl-threat-detector without check
    ${threat_detector_handle} =  Start Process  runuser  -u  sophos-spl-threat-detector  -g  sophos-spl-group  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}
    ...  stderr=STDOUT
    Set Suite Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${threat_detector_handle}
    Register Cleanup   Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    ${td_obj} =  Get Process Object  ${threat_detector_handle}
    Log  Threat Detector Pid = ${td_obj.pid}
    ${result} =  Wait For Process  ${threat_detector_handle}  timeout=${1}
    Run Keyword If  ${result}  Log   ${result.rc}
    Run Keyword If  ${result}  Log   ${result.stdout}

