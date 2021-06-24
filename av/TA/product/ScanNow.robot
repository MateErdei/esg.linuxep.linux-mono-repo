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
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=10
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=20

    Dump Log  ${SCANNOW_LOG_PATH}
    ${scan_now_contents} =  Get File    ${SCANNOW_LOG_PATH}
    Should Contain  ${scan_now_contents}  Excluding directory: /opt/
    Should Contain  ${scan_now_contents}  Excluding directory: /boot/
    Should Contain  ${scan_now_contents}  Excluding directory: /etc/
    Should Contain  ${scan_now_contents}  Excluding directory: /dev/
    Should Contain  ${scan_now_contents}  Excluding file: /absolute_filename_folder/absolute_filename
    Should Contain  ${scan_now_contents}  Excluding file: /relative_filename_folder/relative_filename
    Should Contain  ${scan_now_contents}  Excluding file: /filename_to_exclude
    Should Contain  ${scan_now_contents}  Excluding file: /eicar.star
    Should Contain  ${scan_now_contents}  Excluding file: /eicar.question_mark

    Should Contain  ${scan_now_contents}  Mount point /proc is system and will be excluded from the scan
    Should Contain  ${scan_now_contents}  Excluding mount point: /proc


Scan Now Aborts Scan If Sophos Threat Detector Is Killed And Does Not Recover
    [Timeout]  15min
    Stop AV
    Mark AV Log
    Mark Sophos Threat Detector Log
    Start AV
    register cleanup  Start AV
    register cleanup  Stop AV
    register cleanup  Dump Log  ${SCANNOW_LOG_PATH}

    # Start scan now - abort or timeout...
    Run Scan Now Scan With No Exclusions
    AV Plugin Log Contains With Offset  Received new Action
    Wait Until AV Plugin Log Contains With Offset  Evaluating Scan Now
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=10

    Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    sleep  60  Waiting for the socket to timeout
    Wait Until Keyword Succeeds
    ...  ${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD} secs
    ...  20 secs
    ...  File Log Contains  ${SCANNOW_LOG_PATH}  Reached total maximum number of reconnection attempts. Aborting scan.

    ${line_count} =  Count Lines In Log  ${SCANNOW_LOG_PATH}  Failed to send scan request to Sophos Threat Detector (Environment interruption) - retrying after sleep
    Should Be True   3 <= ${line_count} <= 7

    File Log Contains Once  ${SCANNOW_LOG_PATH}  Reached total maximum number of reconnection attempts. Aborting scan.




Scan Now scans dir with name similar to excluded mount
    Remove Directory  /process  recursive=True
    # configure scan before creating test dir, so that it isn't excluded
    Configure Scan Now Scan
    Register Cleanup  Remove Directory  /process  recursive=True
    Create Directory  /process
    Create File  /process/eicar.com       ${EICAR_STRING}

    Remove File   ${AV_PLUGIN_PATH}/log/Scan Now.log
    Mark AV Log
    Trigger Scan Now Scan
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=240

    AV Plugin Log Contains With Offset  /process/eicar.com
    File Log Contains  ${AV_PLUGIN_PATH}/log/Scan Now.log  "/process/eicar.com" is infected


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
    Require Sophos Threat Detector Running
    register cleanup  Delete Eicars From Tmp
    Check AV Plugin Log exists
    Mark AV Log
    Mark Sophos Threat Detector Log

ScanNow Test Teardown
    Dump Log On Failure   ${AV_LOG_PATH}
    Dump Log On Failure   ${SCANNOW_LOG_PATH}
    Dump Log On Failure   ${FAKEMANAGEMENT_AGENT_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}
    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Error

    run teardown functions

Clear threat detector log
    Remove File   ${THREAT_DETECTOR_LOG_PATH}

Clear logs
    Clear threat detector log

Start AV
    Clear logs
    Remove Files   /tmp/threat_detector.stdout  /tmp/threat_detector.stderr
    ${handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}   stdout=/tmp/threat_detector.stdout  stderr=/tmp/threat_detector.stderr
    Set Suite Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${handle}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed

Stop AV
    ${result} =  Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Dump Log  /tmp/av.stdout
    Dump Log  /tmp/av.stderr
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
