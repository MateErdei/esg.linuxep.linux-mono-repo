*** Settings ***
Documentation   Product tests for AVP
Force Tags      PRODUCT  SCANNOW
Library         Collections
Library         DateTime
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/serialisationtools/CapnpHelper.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/BaseResources.robot
Resource    ../shared/FakeManagementResources.robot
Resource    ../shared/SambaResources.robot

Suite Setup     ScanNow Suite Setup
Suite Teardown  ScanNow Suite Teardown

Test Setup     ScanNow Test Setup
Test Teardown  ScanNow Test Teardown

*** Test Cases ***
Scan Now Honours Exclusions
    Create File    /filename_to_exclude    ${EICAR_STRING}
    Register Cleanup    Remove File  /filename_to_exclude
    Create File    /relative_filename_folder/relative_filename    ${EICAR_STRING}
    Register Cleanup    Remove Directory  /relative_filename_folder/    recursive=True
    Create File    /absolute_filename_folder/absolute_filename    ${EICAR_STRING}
    Register Cleanup    Remove Directory  /absolute_filename_folder/    recursive=True
    Create File    /eicar.star    ${EICAR_STRING}
    Register Cleanup    Remove File  /eicar.star
    Create File    /eicar.question_mark    ${EICAR_STRING}
    Register Cleanup    Remove File  /eicar.question_mark

    Run Scan Now Scan With All Types of Exclusions
    Wait Until AV Plugin Log Contains  Starting scan Scan Now  timeout=10
    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=20

    Dump Log  ${SCANNOW_LOG_PATH}
    ${scan_now_contents} =  Get File    ${SCANNOW_LOG_PATH}
    Should Contain  ${scan_now_contents}  Excluding directory: /bin/
    Should Contain  ${scan_now_contents}  Excluding directory: /boot/
    Should Contain  ${scan_now_contents}  Excluding directory: /etc/
    Should Contain  ${scan_now_contents}  Excluding directory: /dev/
    Should Contain  ${scan_now_contents}  Excluding file: /absolute_filename_folder/absolute_filename
    Should Contain  ${scan_now_contents}  Excluding file: /relative_filename_folder/relative_filename
    Should Contain  ${scan_now_contents}  Excluding file: /filename_to_exclude
    Should Contain  ${scan_now_contents}  Excluding file: /eicar.star
    Should Contain  ${scan_now_contents}  Excluding file: /eicar.question_mark

Scan Now Aborts Scan If Sophos Threat Detector Is Killed And Does Not Recover
    [Timeout]  10min

    ${DETECTOR_BINARY} =   Set Variable   ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector_launcher

    # Start scan now - abort or timeout...
    Run Scan Now Scan With No Exclusions
    AV Plugin Log Contains  Received new Action
    Wait Until AV Plugin Log Contains  Evaluating Scan Now
    Wait Until AV Plugin Log Contains  Starting scan Scan Now  timeout=10

    # Rename the sophos threat detector launcher so that it cannot be restarted
    Move File  ${DETECTOR_BINARY}  ${DETECTOR_BINARY}_moved
    register cleanup  Start AV
    register cleanup  Stop AV
    register cleanup  Move File  ${DETECTOR_BINARY}_moved  ${DETECTOR_BINARY}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  3 secs
    ...  File Log Contains  ${SCANNOW_LOG_PATH}  Scanning
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -SIGSEGV   ${output}
    sleep  60  Waiting for the socket to timeout
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  File Log Contains  ${SCANNOW_LOG_PATH}  Reached total maximum number of reconnection attempts. Aborting scan.

    ${line_count} =  Count Lines In Log  ${SCANNOW_LOG_PATH}  Failed to send scan request to Sophos Threat Detector (Environment interruption) - retrying after sleep
    Should Be Equal As Strings  ${line_count}  9

    File Log Contains Once  ${SCANNOW_LOG_PATH}  Reached total maximum number of reconnection attempts. Aborting scan.

    Dump Log  ${SCANNOW_LOG_PATH}

*** Keywords ***

ScanNow Suite Setup
    Start Fake Management If required
    Start AV
    Check AV Plugin Log exists

ScanNow Suite TearDown
    Stop AV
    Stop Fake Management If Running
    Terminate All Processes  kill=True

ScanNow Test Setup
    Check Sophos Threat Detector Running
    register cleanup  Delete Eicars From Tmp
    Check AV Plugin Log exists

ScanNow Test Teardown
    Dump Log On Failure   ${AV_LOG_PATH}
    Dump Log On Failure   ${SCANNOW_LOG_PATH}
    Dump Log On Failure   ${FAKEMANAGEMENT_AGENT_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}
    run teardown functions

Clear threat detector log
    Remove File   ${THREAT_DETECTOR_LOG_PATH}

Clear logs
    Clear threat detector log

Start AV
    Clear logs
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed

Stop AV
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Dump Log  /tmp/av.stdout
    Dump Log  /tmp/av.stderr
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
