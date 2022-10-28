*** Settings ***

Documentation   Product tests for AVP
Force Tags      PRODUCT  SCANNOW
Library         Collections
Library         DateTime
Library         Process
Library         OperatingSystem
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/serialisationtools/CapnpHelper.py

Resource    ../shared/ErrorMarkers.robot
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

Scan Now Excludes Infected Files Successfully
    # absolute path to a file
    Create File  /eicars/absolute_path_eicar                                                            ${EICAR_STRING}
    Create File  /should_not_be_excluded/eicars/absolute_path_eicar                                     ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /eicars/                               recursive=True

    # absolute path to a directory using wildcard /absolute_directory_with_wildcard/*
    Create File  /absolute_directory_with_wildcard/absolute_path_eicar                                  ${EICAR_STRING}
    Create File  /should_not_be_excluded/absolute_directory_with_wildcard/absolute_path_eicar           ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /absolute_directory_with_wildcard/     recursive=True

    # absolute path to a directory /example/
    Create File  /absolute_directory_without_wildcard/absolute_path_eicar                               ${EICAR_STRING}
    Create File  /should_not_be_excluded/absolute_directory_without_wildcard/absolute_path_eicar        ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /absolute_directory_without_wildcard/  recursive=True

    # Filename with wildcard */eicar2.com
    Create File  /filename_with_wildcard/filename_to_exclude_with_wildcard                              ${EICAR_STRING}
    Create File  /filename_with_wildcard/zfilename_to_exclude_with_wildcard                             ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /filename_with_wildcard/               recursive=True

    # Directory name with wildcard */bar/*
    Create File  /eicars2/directory_name_with_wildcard/eicar.com                                        ${EICAR_STRING}
    Create File  /eicars2/zdirectory_name_with_wildcard/eicar.com                                       ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /eicars2/                              recursive=True

    # Relative path to directory with wildcard */foo/bar/*
    Create File   /eicars3/relative_path_to_directory_with_wildcard/eicar.com                           ${EICAR_STRING}
    Create File   /eicars3/rrelative_path_to_directory_with_wildcard/eicar.com                          ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /eicars3/                              recursive=True

    # Filename without wildcard eicar.com
    Create File  /relative_paths_test_folder/eicar1.com                                                 ${EICAR_STRING}
    Create File  /relative_paths_test_folder/eicar1.comm                                                ${CLEAN_STRING}

    # Directory name without wildcard bar/
    Create File  /relative_paths_test_folder/directory_name_without_wildcard/eicar2.com                 ${EICAR_STRING}
    Create File  /relative_paths_test_folder/ddirectory_name_without_wildcard/eicar2.com                ${CLEAN_STRING}

    # Relative path to directory foo/bar/
    Create File  /relative_paths_test_folder/eicars4/relative_path_to_directory/eicar3.com              ${EICAR_STRING}
    Create File  /relative_paths_test_folder/eeicars4/relative_path_to_directory/eicar3.com             ${CLEAN_STRING}

    # Relative path to file with wildcard */bar/eicar.com
    Create File   /relative_paths_test_folder/relative_path_to_file_with_wildcard/eicar4.com            ${EICAR_STRING}
    Create File   /relative_paths_test_folder/relative_path_to_file_with_wildcard/eicar4.comm           ${CLEAN_STRING}

    # Relative path to file bar/eicar.com
    Create File   /relative_paths_test_folder/relative_path_to_file/eicar5.com                          ${EICAR_STRING}
    Create File   /relative_paths_test_folder/rrelative_path_to_file/eicar5.com                         ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /relative_paths_test_folder/           recursive=True

    # Exclude a file with a .exe extension, in any directory
    Create File  /test/eicar1.exe                                                                       ${EICAR_STRING}
    Create File  /test/eicar2.exe                                                                       ${EICAR_STRING}
    Create File  /test/eicar2.exee                                                                      ${CLEAN_STRING}

    #Exclude an eicar file with any extension, in any directory
    Create File  /test/eicar_extension_exlusion.ts                                                      ${EICAR_STRING}
    Create File  /test/eicar_extension_exlusion.js                                                      ${EICAR_STRING}
    Create File  /test/eicar_extension_exclusion                                                        ${CLEAN_STRING}

    #Absolute path with filename suffix /lib/foo/bar.so
    Create File  /test/test_extension/should_not_be_excluded.so                                         ${EICAR_STRING}
    Create File  /test/test_extension/should_not_be_excluded.soo                                        ${CLEAN_STRING}

    #Absolute path with filename prefix
    Create File  /test/eicar_extension_exlusion2.tsx                                                    ${EICAR_STRING}
    Create File  /test/eicar_extension_exlusion2.jsx                                                    ${EICAR_STRING}
    Create File  /should_not_be_excluded/test/eicar_extension_exlusion2.jsx                             ${CLEAN_STRING}

    #Absolute path with directory name suffix
    Create File  /test/test.so/eicar                                                                    ${EICAR_STRING}
    Create File  /should_not_be_excluded/test/test.so/eicar                                             ${CLEAN_STRING}

    #Absolute path with directory name prefix
    Create File  /test/test2/libz.so/eicar                                                              ${EICAR_STRING}
    Create File  /should_not_be_excluded/test/test2/libz.so/eicar                                       ${CLEAN_STRING}

    #Absolute path with character suffix
    Create File  /test/eicar.1                                                                          ${EICAR_STRING}
    Create File  /should_not_be_excluded/test/eicar.1                                                   ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /should_not_be_excluded/               recursive=True

    #Relative directory with wildcard
    Create File  /test/subpart/partdir/eicar_to_detect                                                  ${EICAR_STRING}
    Create File  /test/ssubpart/partdir/eicar_to_detect                                                 ${CLEAN_STRING}

    #Relative path with wildcard
    Create File  /test/directory/subpart/partdirectory/more/eicar_to_detect.com                         ${EICAR_STRING}
    Create File  /test/ddirectory/subpart/partdirectory/more/eicar_to_detect.com                        ${CLEAN_STRING}
    Register Cleanup    Remove Directory  /test/                                 recursive=True

    Run Scan Now Scan With All Types of Exclusions
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=10
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=20

    Dump Log  ${SCANNOW_LOG_PATH}
    ${scan_now_contents} =  Get File    ${SCANNOW_LOG_PATH}
    Should Contain      ${scan_now_contents}  Excluding directory: /opt/
    Should Contain      ${scan_now_contents}  Excluding directory: /boot/
    Should Contain      ${scan_now_contents}  Excluding directory: /etc/
    Should Contain      ${scan_now_contents}  Excluding directory: /dev/

    Should Contain      ${scan_now_contents}  Excluding file: /eicars/absolute_path_eicar
    Should Not Contain  ${scan_now_contents}  Excluding file: /should_not_be_excluded/eicars/absolute_path_eicar
    Should Contain      ${scan_now_contents}  Excluding directory: /absolute_directory_with_wildcard/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /should_not_be_excluded/absolute_directory_without_wildcard/
    Should Contain      ${scan_now_contents}  Excluding directory: /absolute_directory_without_wildcard/
    Should Contain      ${scan_now_contents}  Excluding file: /filename_with_wildcard/filename_to_exclude_with_wildcard
    Should Not Contain  ${scan_now_contents}  Excluding file: /filename_with_wildcard/zfilename_to_exclude_with_wildcard
    Should Contain      ${scan_now_contents}  Excluding file: /relative_paths_test_folder/eicar1.com
    Should Not Contain  ${scan_now_contents}  Excluding file: /relative_paths_test_folder/eicar1.comm
    Should Contain      ${scan_now_contents}  Excluding directory: /eicars2/directory_name_with_wildcard/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /eicars2/zdirectory_name_with_wildcard/
    Should Contain      ${scan_now_contents}  Excluding directory: /relative_paths_test_folder/directory_name_without_wildcard/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /relative_paths_test_folder/ddirectory_name_without_wildcard/
    Should Contain      ${scan_now_contents}  Excluding directory: /eicars3/relative_path_to_directory_with_wildcard/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /eicars3/rrelative_path_to_directory_with_wildcard/
    Should Contain      ${scan_now_contents}  Excluding directory: /relative_paths_test_folder/eicars4/relative_path_to_directory/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /relative_paths_test_folder/eeicars4/relative_path_to_directory/
    Should Contain      ${scan_now_contents}  Excluding file: /relative_paths_test_folder/relative_path_to_file_with_wildcard/eicar4.com
    Should Not Contain  ${scan_now_contents}  Excluding file: /relative_paths_test_folder/relative_path_to_file_with_wildcard/eicar4.comm
    Should Contain      ${scan_now_contents}  Excluding file: /relative_paths_test_folder/relative_path_to_file/eicar5.com
    Should Not Contain  ${scan_now_contents}  Excluding file: /relative_paths_test_folder/rrelative_path_to_file/eicar5.com
    Should Contain      ${scan_now_contents}  Excluding file: /test/eicar1.exe
    Should Contain      ${scan_now_contents}  Excluding file: /test/eicar2.exe
    Should Not Contain  ${scan_now_contents}  Excluding file: /test/eicar2.exee
    Should Contain      ${scan_now_contents}  Excluding file: /test/eicar_extension_exlusion.ts
    Should Contain      ${scan_now_contents}  Excluding file: /test/eicar_extension_exlusion.js
    Should Not Contain  ${scan_now_contents}  Excluding file: /test/eicar_extension_exclusion
    Should Contain      ${scan_now_contents}  Excluding file: /test/test_extension/should_not_be_excluded.so
    Should Not Contain  ${scan_now_contents}  Excluding file: /test/test_extension/should_not_be_excluded.soo
    Should Contain      ${scan_now_contents}  Excluding file: /test/eicar_extension_exlusion2.tsx
    Should Contain      ${scan_now_contents}  Excluding file: /test/eicar_extension_exlusion2.jsx
    Should Not Contain  ${scan_now_contents}  Excluding file: /should_not_be_excluded/test/eicar_extension_exlusion2.jsx
    Should Contain      ${scan_now_contents}  Excluding directory: /test/test.so/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /should_not_be_excluded/test/test.so/
    Should Contain      ${scan_now_contents}  Excluding directory: /test/test2/libz.so/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /should_not_be_excluded/test/test2/libz.so/
    Should Contain      ${scan_now_contents}  Excluding file: /test/eicar.1
    Should Not Contain  ${scan_now_contents}  Excluding file: /should_not_be_excluded/test/eicar.1
    Should Contain      ${scan_now_contents}  Excluding directory: /test/subpart/partdir/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /test/ssubpart/partdir/
    Should Contain      ${scan_now_contents}  Excluding directory: /test/directory/subpart/partdirectory/
    Should Not Contain  ${scan_now_contents}  Excluding directory: /test/ddirectory/subpart/partdirectory/

    Should Contain      ${scan_now_contents}  Mount point /proc is system and will be excluded from the scan
    Should Contain      ${scan_now_contents}  Excluding mount point: /proc


Scan Now Aborts Scan If Sophos Threat Detector Is Killed And Does Not Recover
    [Timeout]  15min
    Register Cleanup  Exclude Scan Now Terminated
    Register Cleanup  Exclude Unixsocket Failed To Send Scan Request To STD
    Register Cleanup  Exclude Failed To Scan Files
    Register Cleanup  Exclude Aborted Scan Errors

    Register Cleanup  Dump Log  ${SCANNOW_LOG_PATH}

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

    ${line_count} =  Count Lines In Log  ${SCANNOW_LOG_PATH}  Failed to send scan request -
    Should Be True   ${3} <= ${line_count} <= ${7}

    File Log Contains  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Aborting scan, scanner is shutting down
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


Scan Now scan errors do not get logged to av log
    Register Cleanup  Exclude As Corrupted
    Register Cleanup  Exclude As Password Protected
    Remove Directory  /process  recursive=True
    # configure scan before creating test dir, so that it isn't excluded
    Configure Scan Now Scan
    Register Cleanup  Remove Directory  /process  recursive=True
    Create Directory  /process
    Copy File  ${RESOURCES_PATH}/file_samples/passwd-protected.xls  /process
    Copy File  ${RESOURCES_PATH}/file_samples/corrupted.xls  /process

    Remove File   ${AV_PLUGIN_PATH}/log/Scan Now.log
    Mark AV Log
    Trigger Scan Now Scan
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=240
    File Log Contains  ${SCANNOW_LOG_PATH}  Failed to scan /process/passwd-protected.xls as it is password protected
    File Log Contains  ${SCANNOW_LOG_PATH}  Failed to scan /process/corrupted.xls as it is corrupted

    AV Plugin Log Does Not Contain With Offset   Failed to scan


*** Keywords ***

ScanNow Suite Setup
    Start Fake Management If required

ScanNow Suite TearDown
    Stop Fake Management If Running

ScanNow Test Setup
    Start AV
    Component Test Setup
    Require Sophos Threat Detector Running
    Delete Eicars From Tmp
    mark av log
    mark sophos threat detector log
    mark susi debug log

    Register Cleanup      Check All Product Logs Do Not Contain Error
    Register Cleanup      Exclude On Access Scan Errors
    Register Cleanup      Exclude CustomerID Failed To Read Error
    Register Cleanup      Require No Unhandled Exception
    Register Cleanup      Check For Coredumps  ${TEST NAME}
    Register Cleanup      Check Dmesg For Segfaults

ScanNow Test Teardown
    Delete Eicars From Tmp
    #terminates all processes
    Component Test TearDown

    Dump Log On Failure   ${AV_LOG_PATH}
    Dump Log On Failure   ${SCANNOW_LOG_PATH}
    Dump Log On Failure   ${FAKEMANAGEMENT_AGENT_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}

    run teardown functions
    Clear logs

Clear logs
    Remove File   ${THREAT_DETECTOR_LOG_PATH}
    Remove File   ${AV_LOG_PATH}
    Remove File   ${SCANNOW_LOG_PATH}

Start AV
    Clear logs
    Remove Files   /tmp/threat_detector.stdout  /tmp/threat_detector.stderr
    ${handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}   stdout=/tmp/threat_detector.stdout  stderr=/tmp/threat_detector.stderr
    Set Suite Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${handle}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed
