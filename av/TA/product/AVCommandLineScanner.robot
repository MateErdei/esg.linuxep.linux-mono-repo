*** Settings ***
Documentation   Product tests for SSPL-AV Command line scanner
Force Tags      PRODUCT  AVSCANNER  TAP_PARALLEL2
Library         Process
Library         Collections
Library         OperatingSystem
Library         DateTime
Library         ${COMMON_TEST_LIBS}/CoreDumps.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/FileSampleObfuscator.py
Library         ../Libs/FileUtils.py
Library         ../Libs/AVScanner.py
Library         ${COMMON_TEST_LIBS}/OnFail.py
Library         ${COMMON_TEST_LIBS}/OSUtils.py
Library         ${COMMON_TEST_LIBS}/ProcessUtils.py
Library         ../Libs/SystemFileWatcher.py
Library         ../Libs/ThreatReportUtils.py

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Suite Setup     AVCommandLineScanner Suite Setup
Suite Teardown  AVCommandLineScanner Suite TearDown

Test Setup      AVCommandLineScanner Test Setup
Test Teardown   AVCommandLineScanner Test TearDown


*** Keywords ***

AVCommandLineScanner Suite Setup
    Run Keyword And Ignore Error   Empty Directory   ${COMPONENT_ROOT_PATH}/log
    #the above command wont clear the std logs because empty directory cannot remove symlinks
    Run Keyword And Ignore Error   Empty Directory   ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector/
    Run Keyword And Ignore Error   Empty Directory   ${SOPHOS_INSTALL}/tmp
    Remove Directory     ${NORMAL_DIRECTORY}  recursive=True
    Start Fake Management If required
    Create File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

    # Set CORE policy to turn on ML detections
    set_default_policy_from_file  CORE  ${RESOURCES_PATH}/core_policy/CORE-36_oa_disabled.xml
    Copy File   ${RESOURCES_PATH}/susi_settings/susi_settings_ml_on.json  ${SUSI_STARTUP_SETTINGS_FILE}
    Copy File    ${SUSI_STARTUP_SETTINGS_FILE}  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}

    Start AV
    Force SUSI to be initialized
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/disable_safestore


AVCommandLineScanner Suite TearDown
    Stop AV
    Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector
    Stop Fake Management If Running
    Terminate All Processes  kill=True

Reset AVCommandLineScanner Suite
    AVCommandLineScanner Suite TearDown
    AVCommandLineScanner Suite Setup

AVCommandLineScanner Test Setup
    SystemFileWatcher.Start Watching System Files
    Register Cleanup      SystemFileWatcher.stop watching system files

    check av plugin running
    Check Sophos Threat Detector Running

    Create Directory     ${NORMAL_DIRECTORY}
    ${result} =     Check If The Logs Are Close To Rotating
    run keyword if  ${result}   Clear logs

    Register Cleanup      Check All Product Logs Do Not Contain Error
    Register Cleanup      Exclude CustomerID Failed To Read Error
    Register Cleanup      Exclude Expected Sweep Errors
    Register Cleanup      Require No Unhandled Exception

AVCommandLineScanner Test TearDown
    Run Teardown Functions
    Remove Directory      ${NORMAL_DIRECTORY}  recursive=True
    Dump Log On Failure   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log
    Dump Log On Failure   ${FAKEMANAGEMENT_AGENT_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}
    Dump Log On Failure   ${SUSI_DEBUG_LOG_PATH}
    Run Keyword If Test Failed  FakeWatchdog.Dump Log History
    Run Keyword If Test Failed  Reset AVCommandLineScanner Suite

Clear logs
    ${result} =  Run Process  ps  -ef   |    grep   sophos  stderr=STDOUT  shell=yes
    Log  output is ${result.stdout}

    Stop AV
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check AV Plugin Not Running

    Log  Backup logs before removing them
    Dump Log  ${AV_LOG_PATH}
    Dump Log  ${THREAT_DETECTOR_LOG_PATH}
    Dump Log  ${SUSI_DEBUG_LOG_PATH}
    Dump Log  ${SAFESTORE_LOG_PATH}

    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Remove File    ${SUSI_DEBUG_LOG_PATH}
    Remove File    ${SAFESTORE_LOG_PATH}

    Set Suite Variable   ${AV_LOG_MARK}  ${None}
    Set Suite Variable   ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  ${None}
    Set Suite Variable   ${SUSI_DEBUG_LOG_MARK}  ${None}

    Start AV

Stop AV Plugin process
    ${proc} =  Get Process Object  ${AV_PLUGIN_HANDLE}
    Log  Stopping AV Plugin Process PID=${proc.pid}
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr

Start AV Plugin process
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${fake_management_log_path} =   FakeManagementLog.get_fake_management_log_path
    ${fake_management_mark} =  LogUtils.mark_log_size  ${fake_management_log_path}
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    ${proc} =  Get Process Object  ${AV_PLUGIN_HANDLE}
    Log  Started AV Plugin Process PID=${proc.pid}
    Check AV Plugin Installed from Marks  ${fake_management_mark}

Start AV
    Check AV Plugin Not Running
    Check Threat Detector Not Running
    Check Threat Detector PID File Does Not Exist

    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Wait until threat detector running

    Start AV Plugin process

Stop AV
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Stop AV Plugin process


*** Variables ***
${CLEAN_STRING}     not an eicar
${NORMAL_DIRECTORY}     /home/vagrant/this/is/a/directory/for/scanning
${UKNOWN_OPTION_RESULT}      ${2}
${FILE_NOT_FOUND_RESULT}     ${2}
${PERMISSION_DENIED_RESULT}  ${13}
${BAD_OPTION_RESULT}         ${3}
${MANUAL_INTERUPTION_RESULT}         ${41}
${CUSTOM_OUTPUT_FILE}   /home/vagrant/output
${PERMISSIONS_TEST}     ${NORMAL_DIRECTORY}/permissions_test
*** Test Cases ***

CLS No args
    ${result} =  Run Process  ${CLI_SCANNER_PATH}  timeout=120s  stderr=STDOUT

    Log  return code is ${result.rc}
    Log  output is ${result.stdout}

    Should Not Contain  ${result.stdout.replace("\n", " ")}  "failed to execute"

CLS Can Scan Relative Path
    ${cwd} =  get cwd then change directory  ${NORMAL_DIRECTORY}
    Register Cleanup  get cwd then change directory  ${cwd}
    Create Directory  testdir
    Create File     testdir/clean_file     ${CLEAN_STRING}
    Create File     testdir/naughty_eicar  ${EICAR_STRING}

    ${mark} =  Mark Sophos Threat Detector Log

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} testdir

    Log  return code is ${rc}
    Log  output is ${output}
    Should Not Contain  ${output}  Scanning of ${NORMAL_DIRECTORY}/testdir/clean_file was aborted
    Should Not Contain  ${output}  Scanning of ${NORMAL_DIRECTORY}/testdir/naughty_eicar was aborted
    LogUtils.Check Sophos Threat Detector Log Contains After Mark  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/testdir/naughty_eicar (On Demand)  mark=${mark}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Does Not Ordinarily Output To Stderr
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_file 1>/dev/null

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Empty  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Can Scan Infected File
    ${threat_detector_mark} =  Mark Sophos Threat Detector Log
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Wait For Sophos Threat Detector Log Contains After Mark  Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/naughty_eicar (On Demand)  ${threat_detector_mark}


CLS Can Scan Shallow Archive But not Deep Archive
    Register Cleanup      Exclude As Zip Bomb
    Create File     ${NORMAL_DIRECTORY}/archives/eicar    ${EICAR_STRING}
    create archive test files  ${NORMAL_DIRECTORY}/archives

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/archives/eicar2.zip --scan-archives
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/archives/eicar30.zip --scan-archives
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}
    Log  ${output}
    Should Contain  ${output}  Failed to scan ${NORMAL_DIRECTORY}/archives/eicar30.zip
    Should Contain  ${output}  as it is a Zip Bomb

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/archives/eicar15.tar --scan-archives
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/archives/eicar16.tar --scan-archives
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}
    Log  ${output}
    Should Contain  ${output}  Failed to scan ${NORMAL_DIRECTORY}/archives/eicar16.tar
    Should Contain  ${output}  as it is a Zip Bomb


CLS Summary is Correct
    Create Directory  ${NORMAL_DIRECTORY}/EXTRA_FOLDER
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar_2    ${EICAR_STRING}
    Run Process     tar  -cf  ${NORMAL_DIRECTORY}/EXTRA_FOLDER/multiple_eicar.tar  ${NORMAL_DIRECTORY}/naughty_eicar  ${NORMAL_DIRECTORY}/naughty_eicar_2
    Create File     ${NORMAL_DIRECTORY}/EXTRA_FOLDER/clean_file    ${CLEAN_STRING}

    FOR  ${i}  IN RANGE  100
           Create File     ${NORMAL_DIRECTORY}/EXTRA_FOLDER/eicar_${i}    ${EICAR_STRING}
    END

    Check Sophos Threat Detector Running

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/EXTRA_FOLDER /this/file/does/not/exist -a

    Log  ${output}

    Check Sophos Threat Detector Running

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}  102 files scanned in
    Should Contain   ${output}  101 files out of 102 were infected.
    Should Contain   ${output}  102 EICAR-AV-Test infections discovered.
    Should Contain   ${output}  1 scan error encountered


CLS Summary in Less Than a Second
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_file -x ${NORMAL_DIRECTORY}/clean_file

    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain   ${output}  0 files scanned in less than a second.
    Should Contain   ${output}  0 files out of 0 were infected.


CLS Duration Summary is Displayed Correctly
    Start Process    ${CLI_SCANNER_PATH}   /  -x
    ...  /mnt/  file_samples/   /run/docker/netns/  /run/snapd/ns/
    ...  stdout=/tmp/stdout

    Sleep  65s
    Send Signal To Process  SIGINT
    ${result} =  Wait For Process  timeout=10s
    Process Should Be Stopped
    Should Contain   ${result.stdout}  files scanned in 1 minute
    ${seconds} =  Find Value After Phrase  minute,  ${result.stdout}
    Check Is Greater Than  ${seconds}  ${0}


CLS Summary is Printed When Avscanner Is Terminated Prematurely
    Start Process    ${CLI_SCANNER_PATH}   /usr/  stdout=/tmp/stdout  stderr=STDOUT
    Register On Fail  Dump Log  /tmp/stdout
    Register cleanup  Remove File  /tmp/stdout

    sleep  2s
    Send Signal To Process  SIGINT
    ${result} =  Wait For Process  timeout=20s
    Process Should Be Stopped

    ${result} =  Get Process Result

    # May have found a threat or error so can't rely on the return code
    # But we must not get clean/success out
    Should Not Be Equal As Integers	${result.rc}  ${0}

    Should Contain   ${result.stdout}  Received SIGINT
    Should Not Contain  ${result.stdout}  reached the maximum number of connection reconnection attempts. Aborting scan.
    Should Not Contain  ${result.stdout}  Reconnected to Sophos Threat Detector after
    Should Not Contain  ${result.stdout}  - retrying after sleep
    Should Not Contain  ${result.stdout}  Failed to reconnect to Sophos Threat Detector - retrying...

CLS Does not request TFTClassification from SUSI
    ${mark} =  Mark Sophos Threat Detector Log
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}   Detected "${NORMAL_DIRECTORY}/naughty_eicar" is infected with EICAR-AV-Test (On Demand)
    LogUtils.check_sophos_threat_detector_log_does_not_contain_after_mark   TFTClassifications  mark=${mark}


CLS Can Evaluate High Ml Score As A Threat
    run on failure  dump log   ${SUSI_DEBUG_LOG_PATH}
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    ${mark} =  Mark SUSI Debug Log
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/MLengHighScore.exe

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/MLengHighScore.exe" is infected with
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/MLengHighScore.exe" is infected with ML/PE-A (On Demand)

    ${contents} =  LogUtils.get_susi_debug_log_after_mark  ${mark}
    ${primary_score}  ${secondary_score} =  Find Integers After Phrase  ML Scores:  ${contents}
    Check Ml Scores Are Above Threshold  ${primary_score}  ${secondary_score}  ${30}  ${20}

CLS Can Evaluate Low Ml Score As A Clean File
    Copy File  ${RESOURCES_PATH}/file_samples/MLengLowScore.exe  ${NORMAL_DIRECTORY}
    ${mark} =  Mark SUSI Debug Log

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/MLengLowScore.exe

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Not Contain  ${output}  Detected "${NORMAL_DIRECTORY}/MLengLowScore.exe"

    ${contents} =  LogUtils.get_susi_debug_log_after_mark  ${mark}
    ${primary_score}  ${secondary_score} =  Find Integers After Phrase  ML Scores:  ${contents}
    Check Ml Primary Score Is Below Threshold  ${primary_score}  ${30}


CLS Can Scan Archive File
    ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
    Create Directory  ${ARCHIVE_DIR}
    Create File  ${ARCHIVE_DIR}/1_eicar    ${EICAR_STRING}
    Create File  ${ARCHIVE_DIR}/2_clean    ${CLEAN_STRING}
    Create File  ${ARCHIVE_DIR}/3_eicar    ${EICAR_STRING}
    Create File  ${ARCHIVE_DIR}/4_clean    ${CLEAN_STRING}
    Create File  ${ARCHIVE_DIR}/5_eicar    ${EICAR_STRING}

    Run Process     tar  -cf  ${NORMAL_DIRECTORY}/test.tar  ${ARCHIVE_DIR}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/test.tar --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test (On Demand)


CLS Doesnt Detect eicar in zip without archive option
    Create File  ${NORMAL_DIRECTORY}/eicar    ${EICAR_STRING}
    Create Zip   ${NORMAL_DIRECTORY}   eicar   eicar.zip
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar.zip

    Log  return code is ${rc}
    Log  output is ${output}
    Should Not Contain  ${output}  Detected "${NORMAL_DIRECTORY}/eicar.zip/eicar" is infected with EICAR-AV-Test
    Should Not Contain  ${output}  is infected with EICAR-AV-Test (On Demand)
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Can Scan Multiple Archive Files
    ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
    ${SCAN_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/scan_dir

    Create Directory  ${ARCHIVE_DIR}
    Create Directory  ${SCAN_DIR}

    Create File  ${ARCHIVE_DIR}/1_eicar    ${EICAR_STRING}
    Create File  ${ARCHIVE_DIR}/2_clean    ${CLEAN_STRING}
    Create File  ${ARCHIVE_DIR}/3_eicar    ${EICAR_STRING}
    Create File  ${ARCHIVE_DIR}/4_clean    ${CLEAN_STRING}
    Create File  ${ARCHIVE_DIR}/5_eicar    ${EICAR_STRING}

    ${tarresult} =  Run Process     tar  -cf  ${SCAN_DIR}/test.tar  ${ARCHIVE_DIR}   stderr=STDOUT
    ${tgzresult} =  Run Process     tar  -czf  ${SCAN_DIR}/test.tgz  ${ARCHIVE_DIR}   stderr=STDOUT
    ${tarbz2result} =  Run Process     tar  -cjf  ${SCAN_DIR}/test.tar.bz2  ${ARCHIVE_DIR}    stderr=STDOUT

    #Test failed due to very quick scan of archive, check  process succeeded
    Log  ${tarresult.stdout}
    Log  ${tgzresult.stdout}
    Log  ${tarbz2result.stdout}

    Should Be Equal As Integers  ${tarresult.rc}  ${0}
    Should Be Equal As Integers  ${tgzresult.rc}  ${0}
    Should Be Equal As Integers  ${tarbz2result.rc}  ${0}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${SCAN_DIR} --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tgz/Gzip${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tgz/Gzip${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tgz/Gzip${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar.bz2/Bzip2${ARCHIVE_DIR}/1_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar.bz2/Bzip2${ARCHIVE_DIR}/3_eicar" is infected with EICAR-AV-Test (On Demand)
    Should Contain  ${output}  Detected "${SCAN_DIR}/test.tar.bz2/Bzip2${ARCHIVE_DIR}/5_eicar" is infected with EICAR-AV-Test (On Demand)


CLS Reports Reason for Scan Error on Zip Bomb
    Register Cleanup     Exclude As Zip Bomb
    Copy File  ${RESOURCES_PATH}/file_samples/zipbomb.zip  ${NORMAL_DIRECTORY}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/zipbomb.zip --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}
    Should Contain  ${output}  Failed to scan ${NORMAL_DIRECTORY}/zipbomb.zip
    Should Contain  ${output}  as it is a Zip Bomb


CLS Aborts Scanning of Password Protected File
    Register Cleanup     Exclude As Password Protected
    Copy File  ${RESOURCES_PATH}/file_samples/password_protected.7z  ${NORMAL_DIRECTORY}

    ${mark} =  Mark Sophos Threat Detector Log
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/password_protected.7z --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${PASSWORD_PROTECTED_RESULT}
    Should Contain  ${output}  Failed to scan ${NORMAL_DIRECTORY}/password_protected.7z/eicar.com as it is password protected

    LogUtils.dump sophos threat detector log after mark  mark=${mark}
    LogUtils.Check Sophos Threat Detector Log Contains After Mark  ThreatScanner <> Failed to scan ${NORMAL_DIRECTORY}/password_protected.7z/eicar.com as it is password protected  mark=${mark}
    # Keyword wants a list of expected log levels
    verify sophos threat detector log line is level  ${{ ["DEBUG"] }}  ThreatScanner <> Failed to scan ${NORMAL_DIRECTORY}/password_protected.7z/eicar.com as it is password protected  mark=${mark}


CLS Aborts Scanning of Corrupted File
    Register Cleanup     Exclude As Corrupted
    Copy File  ${RESOURCES_PATH}/file_samples/corrupt_tar.tar  ${NORMAL_DIRECTORY}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/corrupt_tar.tar --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}
    # LINUXDAR-4713 - Uncomment below once fixed
    #Should Contain  ${output}  Failed to scan ${NORMAL_DIRECTORY}/corrupt_tar.tar/my.file as it is corrupted
    Should Contain  ${output}  Failed to scan ${NORMAL_DIRECTORY}/corrupt_tar.tar as it is corrupted


CLS Can Report Scan Error And Detection For Archive
    Register Cleanup     Exclude As Password Protected
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/scanErrorAndThreat.tar  ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Failed to scan ${NORMAL_DIRECTORY}/scanErrorAndThreat.tar/EncryptedSpreadsheet.xlsx as it is password protected
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/scanErrorAndThreat.tar/eicar.com" is infected with EICAR-AV-Test (On Demand)



AV Log Contains No Errors When Scanning File
    ${mark} =  Mark AV Log

    Create File     ${NORMAL_DIRECTORY}/naughty_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    wait_for_av_log_contains_after_mark  Found 'EICAR-AV-Test' in '${NORMAL_DIRECTORY}/naughty_eicar'  mark=${mark}
    check_av_log_does_not_contain_after_mark  ERROR  mark=${mark}


CLS Can Scan Infected And Clean File With The Same Name
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Can Detect PUAs
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar_pua    ${EICAR_PUA_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --detect-puas ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar_pua

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar_pua

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Can Detect PsExec
    [Tags]  psexec  disabled
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/PsExec.exe  ${NORMAL_DIRECTORY}/PsExec.exe

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --detect-puas ${NORMAL_DIRECTORY}/PsExec.exe

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}



CLS exclude PUA
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar_pua    ${EICAR_PUA_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --detect-puas ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar_pua --exclude-puas EICAR-PUA-Test

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Will Not Scan Non-Existent File
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/i_do_not_exist

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${FILE_NOT_FOUND_RESULT}
    Should contain  ${output}  1 scan error encountered


CLS Will Not Scan Restricted File
    Register Cleanup  Remove Directory  ${PERMISSIONS_TEST}  true
    Create Directory  ${PERMISSIONS_TEST}
    Create File     ${PERMISSIONS_TEST}/eicar    ${CLEAN_STRING}

    Run  chmod -x ${PERMISSIONS_TEST}

    ${command} =    Set Variable    ${CLI_SCANNER_PATH} ${PERMISSIONS_TEST}/eicar
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${PERMISSION_DENIED_RESULT}


CLS Will Not Scan Inside Restricted Folder
    ${PERMISSIONS_TEST} =  Set Variable  ${NORMAL_DIRECTORY}/permissions_test

    Register Cleanup  Remove Directory  ${PERMISSIONS_TEST}  true
    Create Directory  ${PERMISSIONS_TEST}
    Create File     ${PERMISSIONS_TEST}/eicar    ${CLEAN_STRING}

    Run  chmod -x ${PERMISSIONS_TEST}

    ${command} =    Set Variable    ${CLI_SCANNER_PATH} ${PERMISSIONS_TEST}
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain       ${output.replace("\n", " ")}  Failed to get the symlink status of
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}


CLS Cannot Open Permission Denied File
    Create File     ${NORMAL_DIRECTORY}/eicar    ${CLEAN_STRING}

    Run  chmod -o-r ${NORMAL_DIRECTORY}/eicar

    ${command} =    Set Variable    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain       ${output}  Failed to open as permission denied: /home/vagrant/this/is/a/directory/for/scanning/eicar
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}

CLS Long Threat Paths Are Not Truncated
    # CORE-4999 - threat paths are truncated on SLES
    ${is_suse} =  Does File Contain Word  /etc/os-release    SUSE Linux Enterprise Server
    Pass Execution If    ${is_suse}  There is a bug (CORE-4999) in SUSI which means threat paths get truncated on SLES

    ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
    Create Directory  ${ARCHIVE_DIR}
    Create File  ${ARCHIVE_DIR}/a-long-path-name-long-enough-for-this-test/i-am-a-deep-seated/dir1/dir2/dir3/dir4/dir5/dir6/dir7/dir8/dir9/dir0/dira/dirb/dirc/dird/1_eicar    ${EICAR_STRING}

    Run Process     tar  -cf  ${NORMAL_DIRECTORY}/test.tar  ${ARCHIVE_DIR}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/test.tar --scan-archives

    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar${ARCHIVE_DIR}/a-long-path-name-long-enough-for-this-test/i-am-a-deep-seated/dir1/dir2/dir3/dir4/dir5/dir6/dir7/dir8/dir9/dir0/dira/dirb/dirc/dird/1_eicar" is infected with EICAR-AV-Test (On Demand)


CLS Can Scan Zero Byte File
    Create File  ${NORMAL_DIRECTORY}/zero_bytes
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/zero_bytes

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


# Long Path is 4064 characters long
CLS Can Scan Long Path
    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log   return code is ${rc}
    Log   output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


# Huge Path is over 4064 characters long
CLS Cannot Scan Huge Path
    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/clean_file

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${SCAN_ABORTED}
    Should contain  ${output}  1 scan error encountered


# Huge Path is over 4064 characters long
CLS Can Scan Normal Path But Not SubFolders With a Huge Path
    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  clean_file
    create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  clean_file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${long_path}/

    Log   return code is ${rc}
    Log   output is ${output}
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}


CLS Creates Threat Report
    ${mark} =  Mark AV Log
    Create File     ${NORMAL_DIRECTORY}/clscreatesthreatreport_eicar    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clscreatesthreatreport_eicar

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/clscreatesthreatreport_eicar
    Wait Until AV Plugin Log Contains Detection Event XML After Mark
    ...  mark=${mark}
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${NORMAL_DIRECTORY}/clscreatesthreatreport_eicar


CLS simple encoded eicar
    ${mark} =  Mark AV Log
    Create File  ${NORMAL_DIRECTORY}/脅威    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/脅威" is infected with EICAR-AV-Test
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/脅威


CLS simple encoded eicar in archive
    ${mark} =  Mark AV Log
    Create File  ${NORMAL_DIRECTORY}/脅威    ${EICAR_STRING}
    Run Process     tar  -cf  ${NORMAL_DIRECTORY}/test.tar  -C  ${NORMAL_DIRECTORY}  脅威
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/test.tar --scan-archives
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/test.tar/脅威" is infected with EICAR-AV-Test
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/test.tar


CLS simple eicar in encoded archive
    ${mark} =  Mark AV Log
    Create File  ${NORMAL_DIRECTORY}/eicar    ${EICAR_STRING}
    Run Process     tar  -cf  ${NORMAL_DIRECTORY}/脅威.tar  -C  ${NORMAL_DIRECTORY}  eicar
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/脅威.tar --scan-archives
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}  Detected "${NORMAL_DIRECTORY}/脅威.tar/eicar" is infected with EICAR-AV-Test
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/脅威.tar

CLS Scans DiscImage When Image Setting Is On
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${NORMAL_DIRECTORY}/eicar.iso
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar.iso
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Not Contain  ${output}   Detected "${NORMAL_DIRECTORY}/eicar.iso/1/DIR/subdir/eicar.com" is infected with EICAR-AV-Test


    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar.iso --scan-images
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain  ${output}   Detected "${NORMAL_DIRECTORY}/eicar.iso/1/DIR/subdir/eicar.com" is infected with EICAR-AV-Test

CLS Does Not Scan DiscImage When Archives Setting Is On
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${NORMAL_DIRECTORY}/eicar.iso
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar.iso --scan-archives
    Log  ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Not Contain  ${output}   Detected "${NORMAL_DIRECTORY}/eicar.iso/1/DIR/subdir/eicar.com" is infected with EICAR-AV-Test

CLS Encoded Eicars
    Register Cleanup   Remove Directory  /tmp_test/encoded_eicars  true
    ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh
    Log Many  ${result.stdout}  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}

    ${av_mark} =  Mark AV Log
    ${td_mark} =  Mark Sophos Threat Detector Log

    ${result} =  Run Process  ${CLI_SCANNER_PATH}  /tmp_test/encoded_eicars/  timeout=120s
    Log   ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${VIRUS_DETECTED_RESULT}

    wait_for_av_log_contains_after_mark  Sending threat detection notification to central  mark=${av_mark}
    wait_for_all_eicars_are_reported_in_av_log  /tmp_test/encoded_eicars  mark=${av_mark}

    check_sophos_threat_detector_log_contains_after_mark  Detected "EICAR-AV-Test" in /tmp_test/encoded_eicars/NEWLINEDIR\\n/\\n/bin/sh  mark=${td_mark}
    check_sophos_threat_detector_log_contains_after_mark  Detected "EICAR-AV-Test" in /tmp_test/encoded_eicars/PairDoubleQuote-"VIRUS.com"  mark=${td_mark}
    check_sophos_threat_detector_log_contains_after_mark  ScanningServerConnectionThread scan requested of /tmp_test/encoded_eicars/PairDoubleQuote-"VIRUS.com"  mark=${td_mark}
    check_sophos_threat_detector_log_contains_after_mark  ScanningServerConnectionThread scan requested of /tmp_test/encoded_eicars/NEWLINEDIR\\n/\\n/bin/sh  mark=${td_mark}

    # Check "not contains" at end to give maximum chance for it to be logged
    check_sophos_threat_detector_log_does_not_contain_after_mark  Failed to parse response from SUSI  mark=${td_mark}


CLS Handles Wild Card Eicars
    Create File     ${NORMAL_DIRECTORY}/*   ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    cd ${NORMAL_DIRECTORY} && ${CLI_SCANNER_PATH} "*"
    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/*
    Should Not Contain   ${output}  Scanning ${NORMAL_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output   cd ${NORMAL_DIRECTORY} && ${CLI_SCANNER_PATH} *
    Should Contain      ${output}  Scanning ${NORMAL_DIRECTORY}/*
    Should Contain      ${output}  Scanning ${NORMAL_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Handles Eicar With The Same Name As An Option
    Register Cleanup  Remove File     /-x
    Register Cleanup  Remove File     /--exclude
    Create File     ${NORMAL_DIRECTORY}/--exclude   ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/-x   ${EICAR_STRING}
    Create File     /--exclude   ${EICAR_STRING}
    Create File     /-x   ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/--exclude
    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/--exclude
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/-x
    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/-x
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} ./--exclude
    Should Contain       ${output}  Scanning /--exclude
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} ./-x
    Should Contain       ${output}  Scanning /-x
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} -- -x
    Should Contain       ${output}  Scanning /-x
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  cd / && ${CLI_SCANNER_PATH} -- --exclude
    Should Contain       ${output}  Scanning /--exclude
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Exclusions Filename
    ${TEST_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/testdir
    ${TEST_DIR_ABS} =  Get Substring  ${TEST_DIR}  1

    Create File     ${TEST_DIR}/clean_eicar                                     ${CLEAN_STRING}
    Create File     ${TEST_DIR}/naughty_eicar_folder/eicar                      ${EICAR_STRING}
    Create File     ${TEST_DIR}/clean_eicar_folder/eicar                        ${CLEAN_STRING}
    Create File     ${TEST_DIR}/clean_eicar_folder/eicar.com                    ${CLEAN_STRING}
    #Absolute path with character suffix
    Create File     ${TEST_DIR}/naughty_eicar_folder/eicar.com                  ${EICAR_STRING}
    Create File     ${TEST_DIR}/naughty_eicar_folder/eicar.comm                 ${CLEAN_STRING}
    Create File     ${TEST_DIR}/naughty_eicar_folder/eicar.co                   ${CLEAN_STRING}
    #Absolute path with filename prefix
    Create File     ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.txt      ${EICAR_STRING}
    Create File     ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.exe      ${EICAR_STRING}
    Create File     ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous          ${CLEAN_STRING}
    #Absolute path with filename suffix
    Create File     ${TEST_DIR}/naughty_eicar_folder/bird.js                    ${EICAR_STRING}
    Create File     ${TEST_DIR}/naughty_eicar_folder/exe.js                     ${EICAR_STRING}
    Create File     ${TEST_DIR}/naughty_eicar_folder/clean_file.jss             ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${TEST_DIR}/ --exclude eicar "./${TEST_DIR}/naughty_eicar_folder/eicar.???" "${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.*" "${TEST_DIR}/naughty_eicar_folder/*.js"
    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain       ${output}  Exclusions: eicar, ${TEST_DIR_ABS}/naughty_eicar_folder/eicar.???, ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.*, ${TEST_DIR}/naughty_eicar_folder/*.js,

    Should Contain       ${output}  Scanning ${TEST_DIR}/clean_eicar
    Should Contain       ${output}  Scanning ${TEST_DIR}/clean_eicar_folder/eicar.com
    Should Contain       ${output}  Scanning ${TEST_DIR}/naughty_eicar_folder/eicar.comm
    Should Contain       ${output}  Scanning ${TEST_DIR}/naughty_eicar_folder/eicar.co
    Should Contain       ${output}  Scanning ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous
    Should Not Contain   ${output}  Scanning ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.txt
    Should Not Contain   ${output}  Scanning ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.exe
    Should Contain       ${output}  Scanning ${TEST_DIR}/naughty_eicar_folder/clean_file.jss
    Should Contain       ${output}  Excluding file: ${TEST_DIR}/naughty_eicar_folder/eicar.com
    Should Contain       ${output}  Excluding file: ${TEST_DIR}/naughty_eicar_folder/eicar
    Should Contain       ${output}  Excluding file: ${TEST_DIR}/clean_eicar_folder/eicar
    Should Contain       ${output}  Excluding file: ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.txt
    Should Contain       ${output}  Excluding file: ${TEST_DIR}/naughty_eicar_folder/hi_i_am_dangerous.exe
    Should Contain       ${output}  Excluding file: ${TEST_DIR}/naughty_eicar_folder/bird.js
    Should Contain       ${output}  Excluding file: ${TEST_DIR}/naughty_eicar_folder/exe.js

    Dump Log  ${AV_LOG_PATH}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Relative File Exclusion
    ${TEST_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/testdir
    ${TEST_DIR_WITHOUT_WILDCARD} =  Set Variable  ${NORMAL_DIRECTORY}/testdir/folder_without_wildcard
    ${TEST_DIR_WITH_WILDCARD} =  Set Variable  ${NORMAL_DIRECTORY}/testdir/folder_with_wildcard

    #Relative path to file
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar                                                ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicarr                                ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar                                 ${EICAR_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/naughty_realm/naughty_eicar_folder/eicar                   ${EICAR_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar_folder/eicar                                   ${CLEAN_STRING}
    #Relative path to file with wildcard
    Create File     ${TEST_DIR_WITH_WILDCARD}/wildcard_eicar                                                ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/wildcard_eicar_folder/wildcard_eicarr                         ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/wildcard_eicar_folder/wildcard_eicar                          ${EICAR_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/naughty_realm/wildcard_eicar_folder/wildcard_eicar            ${EICAR_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/clean_eicar_folder/wildcard_eicar                             ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/naughty_eicar_folder/eicar/clean_eicar                        ${CLEAN_STRING}


    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${TEST_DIR}/ --exclude naughty_eicar_folder/eicar "folder_with_wildcard/*/wildcard_eicar"
    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain       ${output}  Exclusions: naughty_eicar_folder/eicar, folder_with_wildcard/*/wildcard_eicar,

    Should Contain       ${output}  Scanning ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar
    Should Contain       ${output}  Scanning ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicarr
    Should Contain       ${output}  Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar
    Should Not Contain   ${output}  Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicarr
    Should Contain       ${output}  Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/naughty_realm/naughty_eicar_folder/eicar
    Should Contain       ${output}  Scanning ${TEST_DIR_WITH_WILDCARD}/wildcard_eicar
    Should Contain       ${output}  Scanning ${TEST_DIR_WITH_WILDCARD}/wildcard_eicar_folder/wildcard_eicarr
    Should Contain       ${output}  Scanning ${TEST_DIR_WITH_WILDCARD}/naughty_eicar_folder/eicar/clean_eicar
    Should Contain       ${output}  Excluding file: ${TEST_DIR_WITH_WILDCARD}/wildcard_eicar_folder/wildcard_eicar
    Should Contain       ${output}  Excluding file: ${TEST_DIR_WITH_WILDCARD}/naughty_realm/wildcard_eicar_folder/wildcard_eicar

    Dump Log  ${AV_LOG_PATH}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Absolute Folder Exclusion
    ${av_mark} =  Mark AV Log
    ${TEST_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/testdir
    ${TEST_DIR_WITHOUT_WILDCARD} =  Set Variable  ${NORMAL_DIRECTORY}/testdir/folder_without_wildcard
    ${TEST_DIR_WITH_WILDCARD} =  Set Variable  ${NORMAL_DIRECTORY}/testdir/folder_with_wildcard

    #Absolute path to directory
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar                    ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar     ${EICAR_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar_folder/eicar       ${CLEAN_STRING}
    #Absolute path to directory with wildcard
    Create File     ${TEST_DIR_WITH_WILDCARD}/clean_eicar                       ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/naughty_eicar_folder/eicar        ${EICAR_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/clean_eicar_folder/eicar          ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${TEST_DIR} --exclude ${TEST_DIR_WITHOUT_WILDCARD}/ "${TEST_DIR_WITH_WILDCARD}/*"
    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain      ${output}  Exclusions: ${TEST_DIR}/folder_without_wildcard/, ${TEST_DIR}/folder_with_wildcard/*,

    Should Contain      ${output}  Excluding directory: ${TEST_DIR_WITHOUT_WILDCARD}/
    Should Contain      ${output}  Excluding directory: ${TEST_DIR_WITH_WILDCARD}/
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITH_WILDCARD}/clean_eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITH_WILDCARD}/naughty_eicar_folder/eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar_folder/eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITH_WILDCARD}/clean_eicar_folder/eicar  mark=${av_mark}

    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Relative Folder Exclusion
    ${av_mark} =  Mark AV Log
    ${TEST_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/testdir
    ${TEST_DIR_WITHOUT_WILDCARD} =  Set Variable  ${NORMAL_DIRECTORY}/testdir/folder_without_wildcard
    ${TEST_DIR_WITH_WILDCARD} =  Set Variable  ${NORMAL_DIRECTORY}/testdir/folder_with_wildcard

    #Relative path to directory
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar                                      ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar                       ${EICAR_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar_folder/eicar                         ${CLEAN_STRING}
    #Relative path to directory with wildcard
    Create File     ${TEST_DIR_WITH_WILDCARD}/dir/subpart/subdir/eicar.com                        ${EICAR_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/ddir/subpart/subdir/eicar.com                       ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/documents/test/subfolder/eicar.com                  ${EICAR_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/ddocuments/test/subfolder/eicar.com                 ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${TEST_DIR} --exclude testdir/folder_without_wildcard/ "dir/su*ir/" "do*er/"
    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain      ${output}  Exclusions: testdir/folder_without_wildcard/, dir/su*ir/, do*er/

    Should Contain      ${output}  Excluding directory: ${TEST_DIR_WITHOUT_WILDCARD}/
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar_folder/eicar  mark=${av_mark}
    Should Contain       ${output}  Excluding directory: ${TEST_DIR_WITH_WILDCARD}/dir/subpart/subdir/
    Should Not Contain   ${output}  Excluding directory: ${TEST_DIR_WITH_WILDCARD}/ddir/subpart/subdir/
    Should Contain       ${output}  Excluding directory: ${TEST_DIR_WITH_WILDCARD}/documents/test/subfolder/
    Should Not Contain   ${output}  Excluding directory: ${TEST_DIR_WITH_WILDCARD}/ddocuments/test/subfolder/

    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Folder Name Exclusion
    ${av_mark} =  Mark AV Log
    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude scanning/
    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain      ${output}  Exclusions: scanning/
    Should Contain      ${output}  Excluding directory: ${NORMAL_DIRECTORY}/
    check_av_log_does_not_contain_after_mark   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar  mark=${av_mark}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Absolute Folder Exclusion And Filename Exclusion
    ${av_mark} =  Mark AV Log
    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude clean_eicar ${NORMAL_DIRECTORY}/clean_eicar_folder/
    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain       ${output}  Exclusions: clean_eicar, ${NORMAL_DIRECTORY}/clean_eicar_folder/,
    Should Contain       ${output}  Scanning ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar
    Should Contain       ${output}  Excluding file: ${NORMAL_DIRECTORY}/clean_eicar
    Should Contain       ${output}  Excluding directory: ${NORMAL_DIRECTORY}/clean_eicar_folder/
    check_av_log_does_not_contain_after_mark   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Excluding file: ${NORMAL_DIRECTORY}/clean_eicar_folder/eicar  mark=${av_mark}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Can Handle Wildcard Exclusions
    Create File     ${NORMAL_DIRECTORY}/exe_eicar.exe    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar.com    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/another_eicar_folder/eicar.com    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/eic.nope    ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude "*.com" "exe_eicar.*" "???.*"
    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain      ${output}  Exclusions: *.com, exe_eicar.*, ???.*,
    Should Contain      ${output}  Excluding file: ${NORMAL_DIRECTORY}/exe_eicar.exe
    Should Contain      ${output}  Excluding file: ${NORMAL_DIRECTORY}/eic.nope
    Should Contain      ${output}  Excluding file: ${NORMAL_DIRECTORY}/naughty_eicar_folder/eicar.com
    Should Contain      ${output}  Excluding file: ${NORMAL_DIRECTORY}/another_eicar_folder/eicar.com
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude "${NORMAL_DIRECTORY}/*/"

    Log  return code is ${rc}
    Log  output is ${output}

    Should Contain      ${output}  Exclusions: ${NORMAL_DIRECTORY}/*/,
    Should Contain      ${output}  Excluding directory: ${NORMAL_DIRECTORY}/naughty_eicar_folder/
    Should Contain      ${output}  Excluding directory: ${NORMAL_DIRECTORY}/another_eicar_folder/
    Should Contain      ${output}  Scanning ${NORMAL_DIRECTORY}/eic.nope
    Should Contain      ${output}  Scanning ${NORMAL_DIRECTORY}/exe_eicar.exe
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Can Handle Relative Non-Canonical Exclusions
    ${test_dir} =  Set Variable  ${NORMAL_DIRECTORY}/exclusion_test_dir/
    Register Cleanup    Remove Directory      ${test_dir}     recursive=True

    Create File     ${test_dir}a/eicar.nope        ${EICAR_STRING}
    Create File     ${test_dir}b/eicar.nope        ${EICAR_STRING}
    Create File     ${test_dir}c/eicar.nope        ${EICAR_STRING}
    Create File     ${test_dir}d/e/eicar.nope      ${EICAR_STRING}
    Create File     ${test_dir}f/g/eicar.nope      ${EICAR_STRING}
    Create File     ${test_dir}h/...nope           ${EICAR_STRING}
    Create File     ${test_dir}i/..nope.           ${EICAR_STRING}
    Create File     ${test_dir}j/...               ${EICAR_STRING}


    ${rc}   ${output} =    Run And Return Rc And Output  cd ${test_dir} && ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude "a//" "b/." "c/../c" "d/./e/" "f/g/.." "./j/..." "i/..nope." "...nope"
    Log   ${rc}
    Log   ${output}
    Should Contain       ${output}   Exclusions: ${test_dir}a/, ${test_dir}b/, ${test_dir}c/, ${test_dir}d/e/, ${test_dir}f/
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Create File     ${test_dir}eicar.nope          ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output  cd ${test_dir} && ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --exclude "../exclusion_test_dir/"
    Log   ${rc}
    Log   ${output}
    Should Contain       ${output}   Exclusions: ${test_dir}
    Should Contain       ${output}   Excluding directory: ${test_dir}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output  ${CLI_SCANNER_PATH} ${test_dir}i/ --exclude ..nope.
    Log   ${rc}
    Log   ${output}
    Should Contain       ${output}   Excluding file: ${test_dir}i/..nope.
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

CLS Can Change Log Level
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --log-level=WARN
    Log   ${output}
    Should Contain       ${output}  Setting logger to log level: WARN

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}
    Log   ${output}
    Should Contain       ${output}  Logger av configured for level: DEBUG

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY} --log-level=ERROR
    Log   ${output}
    Should Contain       ${output}  Setting logger to log level: ERROR


CLS Prints Help and Failure When Options Are Spaced Incorrectly
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --exclude= file
    Should Contain       ${output}   Failed to parse command line options: the argument for option '--exclude' should follow immediately after the equal sign
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --files= file
    Should Contain       ${output}   Failed to parse command line options: the argument for option '--files' should follow immediately after the equal sign
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --config= config
    Should Contain       ${output}   Failed to parse command line options: the argument for option '--config' should follow immediately after the equal sign
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}


CLS Prints Help and Failure When Parsing Incomplete Arguments
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --exclude
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--exclude' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --files
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--files' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --config
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--config' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} -x
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--exclude' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} -f
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--files' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} -c
    Should Contain       ${output}   Failed to parse command line options: the required argument for option '--config' is missing
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}


CLS Can Log To A File
    ${LOG_FILE}      Set Variable   ${NORMAL_DIRECTORY}/scan.log
    ${THREAT_FILE}   Set Variable   ${NORMAL_DIRECTORY}/eicar.com

    Create File     ${THREAT_FILE}    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${THREAT_FILE} --output ${LOG_FILE}

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    File Log Contains    ${LOG_FILE}    "${THREAT_FILE}" is infected with EICAR-AV-Test


CLS Will Not Log To A Directory
    ${THREAT_FILE}   Set Variable   ${NORMAL_DIRECTORY}/eicar.com

    Create File     ${THREAT_FILE}    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${THREAT_FILE} --output ${NORMAL_DIRECTORY}

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${BAD_OPTION_RESULT}

    Should Contain    ${output}    Failed to log to ${NORMAL_DIRECTORY} as it is a directory
    Should Contain    ${output}    Usage: avscanner PATH... [OPTION]...
    Should Not Contain    ${output}    "${THREAT_FILE}" is infected with EICAR-AV-Test


CLS Can Scan Infected File Via Symlink To Directory
    ${td_mark} =  Mark Sophos Threat Detector Log
    ${targetDir} =  Set Variable  ${NORMAL_DIRECTORY}/a/b
    ${sourceDir} =  Set Variable  ${NORMAL_DIRECTORY}/a/c
    Create Directory   ${targetDir}
    Create Directory   ${sourceDir}
    Create File     ${targetDir}/eicar.com    ${EICAR_STRING}
    Create Symlink   ${targetDir}  ${sourceDir}/b
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${sourceDir}/b

    Log  return code is ${rc}
    Log  output is ${output}
    Should Contain       ${output.replace("\n", " ")}  Detected "${sourceDir}/b/eicar.com" (symlinked to ${targetDir}/eicar.com) is infected with EICAR-AV-Test
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    check_sophos_threat_detector_log_contains_after_mark   Detected "EICAR-AV-Test" in ${sourceDir}/b/eicar.com  mark=${td_mark}


CLS Does Not Backtrack Through Symlinks
    ${td_mark} =  Mark Sophos Threat Detector Log
    ${targetDir} =  Set Variable  ${NORMAL_DIRECTORY}/a/b
    ${sourceDir} =  Set Variable  ${NORMAL_DIRECTORY}/a/c
    Create Directory   ${targetDir}
    Create Directory   ${sourceDir}
    Create File     ${targetDir}/eicar.com    ${EICAR_STRING}
    Create Symlink  ${targetDir}  ${sourceDir}/b
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${sourceDir}/b ${targetDir}

    Log  return code is ${rc}
    Log  output is ${output}
    Should Contain       ${output.replace("\n", " ")}  Detected "${sourceDir}/b/eicar.com" (symlinked to ${targetDir}/eicar.com) is infected with EICAR-AV-Test
    Should Not Contain   ${output.replace("\n", " ")}  Detected "${targetDir}/eicar.com" is infected with EICAR-AV-Test
    Should Contain       ${output.replace("\n", " ")}  Directory already scanned: "${targetDir}"
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    check_sophos_threat_detector_log_contains_after_mark   Detected "EICAR-AV-Test" in ${sourceDir}/b/eicar.com  mark=${td_mark}


CLS Can Scan Infected File Via Symlink To File
    ${td_mark} =  Mark Sophos Threat Detector Log
    Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}
    Create Symlink  ${NORMAL_DIRECTORY}/eicar.com  ${NORMAL_DIRECTORY}/symlinkToEicar
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/symlinkToEicar

    Log  return code is ${rc}
    Log  output is ${output}
    Should Contain       ${output.replace("\n", " ")}  Detected "${NORMAL_DIRECTORY}/symlinkToEicar" (symlinked to ${NORMAL_DIRECTORY}/eicar.com) is infected with EICAR-AV-Test
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    check_sophos_threat_detector_log_contains_after_mark   Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/symlinkToEicar  mark=${td_mark}


CLS Can Scan Infected File Via Symlink To Directory Containing File
    ${td_mark} =  Mark Sophos Threat Detector Log
    Create Directory    ${NORMAL_DIRECTORY}/a
    Create File         ${NORMAL_DIRECTORY}/a/eicar.com    ${EICAR_STRING}
    Create Symlink  ${NORMAL_DIRECTORY}/a  ${NORMAL_DIRECTORY}/symlinkToDir
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/symlinkToDir --follow-symlinks

    Log  return code is ${rc}
    Log  output is ${output}
    Should Contain       ${output.replace("\n", " ")}  Detected "${NORMAL_DIRECTORY}/symlinkToDir/eicar.com" (symlinked to ${NORMAL_DIRECTORY}/a/eicar.com) is infected with EICAR-AV-Test
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    check_sophos_threat_detector_log_contains_after_mark   Detected "EICAR-AV-Test" in ${NORMAL_DIRECTORY}/symlinkToDir/eicar.com  mark=${td_mark}


CLS Skips The Scanning Of Symlink Targets On Special Mount Points
    File Should Exist        /proc/uptime
    Create Symlink           /proc/uptime  ${NORMAL_DIRECTORY}/symlinkToProcFile
    Directory Should Exist   /proc/tty
    Create Symlink           /proc/tty  ${NORMAL_DIRECTORY}/symlinkToProcDir

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${NORMAL_DIRECTORY}
    Log  output is ${output}

    Should Not Contain   ${output}  Scanning ${NORMAL_DIRECTORY}/symlinkToProcFile
    Should Not Contain   ${output}  Scanning /proc/uptime
    Should Contain       ${output}  Skipping the scanning of symlink target ("/proc/uptime") which is on excluded mount point: /proc

    Should Not Contain   ${output}  Scanning ${NORMAL_DIRECTORY}/symlinkToProcDir
    Should Not Contain   ${output}  Scanning /proc/tty
    Should Contain       ${output}  Skipping the scanning of symlink target ("/proc/tty") which is on excluded mount point: /proc

    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}


CLS Can Exclude Scanning of Symlink To File
    ${targetDir1} =  Set Variable  ${NORMAL_DIRECTORY}/target_dir1
    ${targetDir2} =  Set Variable  ${NORMAL_DIRECTORY}/target_dir2
    ${sourceDir} =  Set Variable  ${NORMAL_DIRECTORY}/source_dir
    Create Directory   ${targetDir1}
    Create Directory   ${targetDir2}
    Create Directory   ${sourceDir}
    Create File     ${targetDir1}/eicar.com    ${EICAR_STRING}
    Create File     ${targetDir2}/eicar.com    ${EICAR_STRING}
    Create Symlink  ${targetDir1}/eicar.com  ${sourceDir}/absolute_link
    Create Symlink  ../target_dir2/eicar.com  ${sourceDir}/relative_link

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude fee/ fi ${NORMAL_DIRECTORY}/fo/ ${NORMAL_DIRECTORY}/fum
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain       ${output}  Detected "${sourceDir}/absolute_link" (symlinked to ${targetDir1}/eicar.com) is infected with EICAR-AV-Test
    Should Contain       ${output}  Detected "${sourceDir}/relative_link" (symlinked to ${targetDir2}/eicar.com) is infected with EICAR-AV-Test

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude absolute_link relative_link
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Excluding symlinked file: ${sourceDir}/absolute_link
    Should Contain       ${output}  Excluding symlinked file: ${sourceDir}/relative_link

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude ${sourceDir}/absolute_link ${sourceDir}/relative_link
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Excluding symlinked file: ${sourceDir}/absolute_link
    Should Contain       ${output}  Excluding symlinked file: ${sourceDir}/relative_link

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude eicar.com
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir1}/eicar.com") which is excluded by user defined exclusion: eicar.com
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir2}/eicar.com") which is excluded by user defined exclusion: eicar.com

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude ${targetDir1}/eicar.com ${targetDir2}/eicar.com
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir1}/eicar.com") which is excluded by user defined exclusion: ${targetDir1}/eicar.com
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir2}/eicar.com") which is excluded by user defined exclusion: ${targetDir2}/eicar.com

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude target_dir1/ target_dir2/
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir1}/eicar.com") which is excluded by user defined exclusion: target_dir1/
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir2}/eicar.com") which is excluded by user defined exclusion: target_dir2/

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude ${targetDir1}/ ${targetDir2}/
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir1}/eicar.com") which is excluded by user defined exclusion: ${targetDir1}/
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir2}/eicar.com") which is excluded by user defined exclusion: ${targetDir2}/


CLS Can Exclude Scanning of Symlink To Folder
    ${targetDir1} =  Set Variable  ${NORMAL_DIRECTORY}/target_dir1
    ${targetDir2} =  Set Variable  ${NORMAL_DIRECTORY}/target_dir2
    ${sourceDir} =  Set Variable  ${NORMAL_DIRECTORY}/source_dir
    Create Directory   ${targetDir1}
    Create Directory   ${targetDir2}
    Create Directory   ${sourceDir}
    Create File     ${targetDir1}/eicar.com    ${EICAR_STRING}
    Create File     ${targetDir2}/eicar.com    ${EICAR_STRING}
    Create Symlink  ${targetDir1}  ${sourceDir}/absolute_link
    Create Symlink  ../target_dir2  ${sourceDir}/relative_link

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude fee/ fi ${NORMAL_DIRECTORY}/fo/ ${NORMAL_DIRECTORY}/fum
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain       ${output}  Detected "${sourceDir}/absolute_link/eicar.com" is infected with EICAR-AV-Test
    Should Contain       ${output}  Detected "${sourceDir}/relative_link/eicar.com" is infected with EICAR-AV-Test

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude absolute_link/ relative_link/
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Excluding directory: ${sourceDir}/absolute_link
    Should Contain       ${output}  Excluding directory: ${sourceDir}/relative_link

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude ${sourceDir}/absolute_link/ ${sourceDir}/relative_link/
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Excluding directory: ${sourceDir}/absolute_link
    Should Contain       ${output}  Excluding directory: ${sourceDir}/relative_link

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude target_dir1/ target_dir2/
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir1}") which is excluded by user defined exclusion: target_dir1/
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir2}") which is excluded by user defined exclusion: target_dir2/

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${sourceDir} --exclude ${targetDir1}/ ${targetDir2}/
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir1}") which is excluded by user defined exclusion: ${targetDir1}/
    Should Contain       ${output}  Skipping the scanning of symlink target ("${targetDir2}") which is excluded by user defined exclusion: ${targetDir2}/


CLS Reports Error Once When Using Custom Log File
    Remove File  ${CUSTOM_OUTPUT_FILE}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} "file_does_not_exist" --output ${CUSTOM_OUTPUT_FILE}
    ${content} =  Get File   ${CUSTOM_OUTPUT_FILE}  encoding_errors=replace
    ${lines} =  Get Lines Containing String     ${content}  file/folder does not exist

    ${count} =  Get Line Count   ${lines}
    Should Be Equal As Integers  ${1}  ${count}


CLS Scans root with non-canonical path

    ${av_mark} =  Mark AV Log
    ${exclusions} =  ExclusionHelper.Get Root Exclusions for avscanner except proc
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /. -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /./ -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /boot/.. -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /.. -x ${exclusions}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} / -x ${exclusions}

    check_av_log_does_not_contain_after_mark   Scanning /proc/  mark=${av_mark}
    check_av_log_does_not_contain_after_mark   Scanning /./proc/  mark=${av_mark}


CLS Scans Paths That Exist and Dont Exist
    Create File     ${NORMAL_DIRECTORY}/clean_eicar    ${CLEAN_STRING}
    Create File     ${NORMAL_DIRECTORY}/.clean_eicar_folder/eicar    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/.clean_eicar_folder/eicar ${NORMAL_DIRECTORY}/.dont_exist/eicar ${NORMAL_DIRECTORY}/.doesnot_exist ${NORMAL_DIRECTORY}/clean_eicar
    Log     ${output}

    Should Contain      ${output}  Scanning /home/vagrant/this/is/a/directory/for/scanning/.clean_eicar_folder/eicar
    Should Contain      ${output}  Failed to scan "/home/vagrant/this/is/a/directory/for/scanning/.dont_exist/eicar": file/folder does not exist
    Should Contain      ${output}  Failed to scan "/home/vagrant/this/is/a/directory/for/scanning/.doesnot_exist": file/folder does not exist
    Should Contain      ${output}  Scanning /home/vagrant/this/is/a/directory/for/scanning/clean_eicar

    Should Be Equal As Integers  ${rc}  ${FILE_NOT_FOUND_RESULT}


CLS Scans file on NFS
    [Tags]  NFS
    ${source} =        Set Variable  /tmp_test/nfsshare
    ${destination} =   Set Variable  /mnt/nfsshare

    Remove Directory   /tmp_test   recursive=true
    Register Cleanup   Remove Directory   /tmp_test   recursive=true
    Create Directory   ${source}
    Create File        ${source}/eicar.com    ${EICAR_STRING}
    Create Directory   ${destination}
    Create Local NFS Share   ${source}   ${destination}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${destination}
    Log     ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain     ${output}   Detected "${destination}/eicar.com" is infected with EICAR-AV-Test


CLS Reconnects And Continues Scan If Sophos Threat Detector Is Restarted
    ${LOG_FILE} =          Set Variable   ${NORMAL_DIRECTORY}/scan.log

    ${cli_mark} =  Mark Log Size   ${LOG_FILE}
    ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   /  -x  /mnt/  file_samples/  /run/snapd/ns/  stdout=${LOG_FILE}   stderr=STDOUT
    Wait For Log Contains After Mark  ${LOG_FILE}  Scanning  ${cli_mark}
    Stop AV
    Start AV
    Wait For Log Contains After Mark  ${LOG_FILE}  Reconnected to Sophos Threat Detector  ${cli_mark}
    Check Log Does Not Contain After Mark  ${LOG_FILE}  reached the maximum number of connection reconnection attempts. Aborting scan.  ${cli_mark}

    ${cli_mark2} =  Mark Log Size   ${LOG_FILE}
    Wait For Log Contains After Mark  ${LOG_FILE}  Scanning  ${cli_mark2}

    Process should Be Running   handle=${HANDLE}
    ${result} =   Terminate Process   handle=${HANDLE}
    Sleep   0.5   wait for threat detector logs to catch up

CLS scan with Bind Mount
    ${source} =       Set Variable  /tmp_test/directory
    ${destination} =  Set Variable  /tmp_test/bind_mount
    Register Cleanup  Remove Directory   /tmp_test   recursive=true
    Remove Directory  /tmp_test   recursive=true
    Create Directory  ${source}
    Create Directory  ${destination}
    Create File       ${source}/eicar.com    ${EICAR_STRING}

    Run Shell Process   mount --bind ${source} ${destination}     OnError=Failed to create bind mount
    Register Cleanup  Unmount Test Mount  ${destination}

    Should Exist      ${destination}/eicar.com

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/
    Log  return code is ${rc}
    Log  output is ${output}

    ${lines} =        Get Lines Containing String    ${output}   is infected with EICAR-AV-Test
    ${count} =        Get Line Count   ${lines}
    Should Be Equal As Integers  ${1}  ${count}


CLS scans dir with name similar to excluded mount
    ${testdir1} =       Set Variable   ${NORMAL_DIRECTORY}/testdir1
    ${testdir2} =       Set Variable   ${NORMAL_DIRECTORY}/testdir2
    Create Directory    ${testdir1}/proc
    Run Shell Process   mount -t proc proc ${testdir1}/proc     OnError=Failed to create proc mount
    Register Cleanup    Run Shell Process   umount ${testdir1}/proc   OnError=Failed to release proc mount
    Should Exist        ${testdir1}/proc/uptime

    Create Directory    ${testdir1}/process
    Create File         ${testdir1}/process/eicar.com    ${EICAR_STRING}
    Create File         ${testdir1}/process_eicar.com    ${EICAR_STRING}
    Create Symlink      ${testdir2}/eicar.com   ${testdir1}/process_symlink

    Create Directory    ${testdir2}
    Create File         ${testdir2}/eicar.com   ${EICAR_STRING}
    Create Symlink      ${testdir1}/process_eicar.com   ${testdir2}/file_link
    Create Symlink      ${testdir1}/process   ${testdir2}/dir_link

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${testdir1}
    Log                 return code is ${rc}
    Log                 output is ${output}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain      ${output}   Detected "${testdir1}/process/eicar.com" is infected with EICAR-AV-Test (On Demand)
    Should Contain      ${output}   Detected "${testdir1}/process_eicar.com" is infected with EICAR-AV-Test (On Demand)
    Should Contain      ${output}   Detected "${testdir1}/process_symlink" (symlinked to ${testdir2}/eicar.com) is infected with EICAR-AV-Test (On Demand)

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} --follow-symlinks ${testdir2}
    Log                 return code is ${rc}
    Log                 output is ${output}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain      ${output}   Detected "${testdir2}/eicar.com" is infected with EICAR-AV-Test
    Should Contain      ${output}   Detected "${testdir2}/file_link" (symlinked to ${testdir1}/process_eicar.com) is infected with EICAR-AV-Test (On Demand)
    Should Contain      ${output}   Detected "${testdir2}/dir_link/eicar.com" is infected with EICAR-AV-Test


CLS scan with ISO mount
    ${source} =       Set Variable  /tmp_test/iso_test/eicar.iso
    ${destination} =  Set Variable  /tmp_test/iso_test/iso_mount
    Register Cleanup  Remove Directory   /tmp_test   recursive=true
    Remove Directory  /tmp_test   recursive=true
    Create Directory  ${destination}
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${source}
    Run Shell Process   mount -o ro,loop ${source} ${destination}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination}   OnError=Failed to release loopback mount
    Should Exist      ${destination}/DIR/subdir/eicar.com

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/iso_test/
    Log  return code is ${rc}
    Log  output is ${output}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}   Detected "${destination}/DIR/subdir/eicar.com" is infected with EICAR-AV-Test (On Demand)


CLS scan two mounts same inode numbers
    # Mount two copies of the same iso file. inode numbers on the mounts will be identical, but device numbers should
    # differ. We should walk both mounts.
    ${source} =       Set Variable  /tmp_test/inode_test/eicar.iso
    ${destination} =  Set Variable  /tmp_test/inode_test/iso_mount
    Register Cleanup  Remove Directory   /tmp_test   recursive=true
    Remove Directory  /tmp_test   recursive=true
    Create Directory  ${destination}
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${source}
    Run Shell Process   mount -o ro,loop ${source} ${destination}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination}   OnError=Failed to release loopback mount
    Should Exist      ${destination}/DIR/subdir/eicar.com

    ${source2} =       Set Variable  /tmp_test/inode_test/eicar2.iso
    ${destination2} =  Set Variable  /tmp_test/inode_test/iso_mount2
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${source2}
    Register Cleanup  Remove File   ${source2}
    Create Directory  ${destination2}
    Run Shell Process   mount -o ro,loop ${source2} ${destination2}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination2}   OnError=Failed to release loopback mount
    Should Exist      ${destination2}/DIR/subdir/eicar.com

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/inode_test/
    Log  return code is ${rc}
    Log  output is ${output}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}   Detected "${destination}/DIR/subdir/eicar.com" is infected with EICAR-AV-Test (On Demand)
    Should Contain   ${output}   Detected "${destination2}/DIR/subdir/eicar.com" is infected with EICAR-AV-Test (On Demand)


CLS Can Scan Infected And Error Files
    Register Cleanup  Exclude As Zip Bomb
    Copy File  ${RESOURCES_PATH}/file_samples/zipbomb.zip  ${NORMAL_DIRECTORY}
    Create File  ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}

    ${rc}   ${output} =  Run And Return Rc And Output  ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/eicar.com ${NORMAL_DIRECTORY}/zipbomb.zip --scan-archives
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}



CLS Can Scan Clean And Error Files
    Register Cleanup  Exclude As Zip Bomb
    Copy File  ${RESOURCES_PATH}/file_samples/zipbomb.zip  ${NORMAL_DIRECTORY}
    Create File  ${NORMAL_DIRECTORY}/cleanfile.txt    ${CLEAN_STRING}

    ${rc}   ${output} =  Run And Return Rc And Output  ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/cleanfile.txt ${NORMAL_DIRECTORY}/zipbomb.zip --scan-archives
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}


CLS Can Scan PE Files without Crashing
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    ${rc}   ${output} =  Run And Return Rc And Output  ${CLI_SCANNER_PATH} ${RESOURCES_PATH}/file_samples/CertMgr.Exe

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}


CLS Skips scanning of special file
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /dev/null
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Contain   ${output}   Not scanning special file/device: "/dev/null"

CLS Return Codes Are Correct
    Register Cleanup  Exclude As Password Protected
    Register Cleanup  Exclude As Corrupted

    Create File     ${NORMAL_DIRECTORY}/dirty_file    ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_file
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Copy File  ${RESOURCES_PATH}/file_samples/password_protected.7z  ${NORMAL_DIRECTORY}
    Copy File  ${RESOURCES_PATH}/file_samples/corrupt_tar.tar  ${NORMAL_DIRECTORY}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/password_protected.7z --scan-archives
    Should Be Equal As Integers  ${rc}  ${PASSWORD_PROTECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/password_protected.7z --scan-archives
    Should Be Equal As Integers  ${rc}  ${PASSWORD_PROTECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/corrupt_tar.tar --scan-archives
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/corrupt_tar.tar ${NORMAL_DIRECTORY}/password_protected.7z --scan-archives
    Should Be Equal As Integers  ${rc}  ${PASSWORD_PROTECTED_RESULT}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/ ${NORMAL_DIRECTORY}/corrupt_tar.tar ${NORMAL_DIRECTORY}/password_protected.7z --scan-archives
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}


CLS Can Append Summary To Log When SigTerm Occurs
    ${SCAN_LOG} =    Set Variable    /tmp/sigterm_test.log
    Register Cleanup  Remove File  ${SCAN_LOG}
    Register On Fail  Dump Log  ${SCAN_LOG}
    Remove File  ${SCAN_LOG}

    ${SCAN_OUT} =    Set Variable    /tmp/sigterm_out.log
    Register Cleanup  Remove File  ${SCAN_OUT}
    Register On Fail   Dump Log   ${SCAN_OUT}
    Remove File  ${SCAN_OUT}

    Register Cleanup  Exclude Scan Errors From File Samples

    ${cls_handle} =     Start Process
        ...   ${CLI_SCANNER_PATH}  -o  ${SCAN_LOG}  /  -x  /mnt/  /run/snapd/ns/
        ...   stdout=${SCAN_OUT}  stderr=STDOUT
    ${cls_pid} =   Get Process Id   handle=${cls_handle}
    Log  PID: ${cls_pid}
    Register cleanup  Run Keyword And Ignore Error   Terminate Process  handle=${cls_handle}  kill=True

    Wait Until File exists  ${SCAN_LOG}
    Wait For File With Particular Contents   \ Scanning\    ${SCAN_LOG}
    Dump Log  ${SCAN_LOG}

    dump_threads_from_pid  ${cls_pid}
    register on fail  dump_threads_from_pid  ${cls_pid}

    Send Signal To Process  SIGTERM  handle=${cls_handle}
    ${result} =  Wait For Process    handle=${cls_handle}  timeout=10  on_timeout=continue
    Process Should Be Stopped   handle=${cls_handle}

    Wait For File With Particular Contents  Scan aborted due to environment interruption  ${SCAN_LOG}  timeout=1
    Check Specific File Content    End of Scan Summary:  ${SCAN_LOG}


CLS Can Append Summary To Log When SigTerm Occurs Strace
    [Tags]  STRACE  MANUAL
    ${SCAN_LOG} =    Set Variable    /tmp/sigterm_test.log
    Register Cleanup  Remove File  ${SCAN_LOG}
    Register Cleanup  Exclude Scan Errors From File Samples
    Remove File  ${SCAN_LOG}
    Remove File  /tmp/sigterm_test_strace.log
    ${cls_handle} =     Start Process    strace  -f  -o  /tmp/sigterm_test_strace.log  ${CLI_SCANNER_PATH}  -o  ${SCAN_LOG}  /  -x  /mnt/  /run/snapd/ns/
    Register cleanup  Run Keyword And Ignore Error  Terminate Process  handle=${cls_handle}  kill=True
    register on fail  Dump Log  ${SCAN_LOG}
    register on fail  Dump Log  /tmp/sigterm_test_strace.log

    Wait Until File exists  ${SCAN_LOG}
    Dump Log  ${SCAN_LOG}

    Send Signal To Process  SIGTERM  handle=${cls_handle}  group=True
    ${result} =  Wait For Process    handle=${cls_handle}  timeout=30  on_timeout=terminate
    ## The process has written everything is will
    Wait For File With Particular Contents  Scan aborted due to environment interruption  ${SCAN_LOG}  timeout=1

    Check Specific File Content    End of Scan Summary:  ${SCAN_LOG}
    Dump Log  /tmp/sigterm_test_strace.log

CLS Can Append Summary To Log When SIGHUP Is Received
    ${SCAN_LOG} =    Set Variable    /tmp/sighup_test.log
    Register Cleanup  Remove File  ${SCAN_LOG}
    Register On Fail  Dump Log  ${SCAN_LOG}
    Remove File  ${SCAN_LOG}

    ${SCAN_OUT} =    Set Variable    /tmp/sighup_out.log
    Register Cleanup  Remove File  ${SCAN_OUT}
    Register On Fail   Dump Log   ${SCAN_OUT}
    Remove File  ${SCAN_OUT}

    Register Cleanup  Exclude Scan Errors From File Samples

    ${td_mark} =  Mark Sophos Threat Detector Log
    ${cls_handle} =     Start Process
        ...   ${CLI_SCANNER_PATH}  -o  ${SCAN_LOG}  /  -x  /mnt/  /run/snapd/
        ...   stdout=${SCAN_OUT}  stderr=STDOUT
    ${cls_pid} =   Get Process Id   handle=${cls_handle}
    Log  PID: ${cls_pid}
    Register cleanup  Run Keyword And Ignore Error   Terminate Process  handle=${cls_handle}  kill=True

    Wait Until File exists  ${SCAN_LOG}
    Wait For File With Particular Contents   \ Scanning\    ${SCAN_LOG}
    Dump Log  ${SCAN_LOG}

    dump_threads_from_pid  ${cls_pid}
    register on fail  dump_threads_from_pid  ${cls_pid}

    Send Signal To Process  SIGHUP  handle=${cls_handle}
    ${result} =  Wait For Process    handle=${cls_handle}  timeout=10  on_timeout=continue
    Process Should Be Stopped   handle=${cls_handle}

    Check Specific File Content    Scan aborted due to environment interruption  ${SCAN_LOG}
    Check Specific File Content    End of Scan Summary:  ${SCAN_LOG}

    wait_for_log_contains_from_mark  ${td_mark}  Stopping ScanningServerConnectionThread

CLS Can Append Summary To Log When SIGHUP Is Received strace
    [Tags]  STRACE   MANUAL
    ${SCAN_LOG} =    Set Variable    /tmp/sighup_test.log
    Register Cleanup  Remove File  ${SCAN_LOG}
    Remove File  ${SCAN_LOG}
    Remove File  /tmp/sighup_test_strace.log

    ${td_mark} =  Mark Sophos Threat Detector Log
    ${cls_handle} =     Start Process    strace  -f  -o  /tmp/sighup_test_strace.log  ${CLI_SCANNER_PATH}  -o  ${SCAN_LOG}  /  -x  /mnt/  /run/snapd/
    Register cleanup  Run Keyword And Ignore Error  Terminate Process  handle=${cls_handle}  kill=True
    register on fail  Dump Log  ${SCAN_LOG}
    register on fail  Dump Log  /tmp/sigterm_test_strace.log

    Wait Until File exists  ${SCAN_LOG}
    Wait For File With Particular Contents   \ Scanning\   ${SCAN_LOG}

    Send Signal To Process  SIGHUP  handle=${cls_handle}  group=True
    ${result} =  Wait For Process    handle=${cls_handle}  timeout=30  on_timeout=terminate
    Dump Log  ${SCAN_LOG}
    Dump Log  /tmp/sigterm_test_strace.log

    Check Specific File Content    Scan aborted due to environment interruption  ${SCAN_LOG}
    Check Specific File Content    End of Scan Summary:  ${SCAN_LOG}

    wait_for_log_contains_from_mark  ${td_mark}  Stopping ScanningServerConnectionThread

CLS Can Complete A Scan Despite Specified Log File Being Read-Only
    Register Cleanup  Remove File  /tmp/scan.log
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/naughty_eicar

    ${av_mark} =  Mark AV Log
    Create File  ${NORMAL_DIRECTORY}/naughty_eicar  ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar -o /tmp/scan.log

    ${result} =  Run Process  ls  -l  /tmp/scan.log
    Log  Old permissions: ${result.stdout}

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    File Log Contains  /tmp/scan.log  Detected "${NORMAL_DIRECTORY}/naughty_eicar" is infected with EICAR-AV-Test (On Demand)
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar

    ${av_mark} =  Mark AV Log
    ${scan_mark} =  mark_log_size  /tmp/scan.log
    Run  chmod 444 /tmp/scan.log

    ${result} =  Run Process  ls  -l  /tmp/scan.log
    Log  New permissions: ${result.stdout}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/naughty_eicar

    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    check_log_does_not_contain_after_mark  /tmp/scan.log  Detected "${NORMAL_DIRECTORY}/naughty_eicar" is infected with EICAR-AV-Test  ${scan_mark}
    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/naughty_eicar

CLS Can Scan Clean File Twice Faster Second time
    Stop AV
    Start AV
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${before} =  Get Time  epoch
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_file
    ${after1} =  Get Time  epoch
    ${rc2}   ${output2} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/clean_file
    ${after2} =  Get Time  epoch

    Log  return code is ${rc}
    Log  output is ${output}
    Should Not Contain  ${output}  Scanning of ${NORMAL_DIRECTORY}/clean_file was aborted
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Be Equal As Integers  ${rc2}  ${CLEAN_RESULT}
    ${duration1} =  Evaluate  ${after1} - ${before}
    ${duration2} =  Evaluate  ${after2} - ${after1}
    Log  First duration is ${duration1}
    Log  Second duration is ${duration2}
    Run Keyword if  ${duration1} < ${duration2}  Fail  First scan quicker ${duration1} than second scan ${duration2}

CLS Can init susi safely in parallel
    ## Ensure SUSI not initialized
    register cleanup  dump log   ${THREAT_DETECTOR_LOG_PATH}
    Stop AV
    Start AV
    Create File     ${NORMAL_DIRECTORY}/clean_file    ${CLEAN_STRING}
    ${process1} =   Start Process   ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/clean_file
    ${process2} =   Start Process   ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/clean_file
    ${result1} =    Wait For Process   ${process1}  timeout=${60}  on_timeout=kill
    ${result2} =    Wait For Process   ${process2}  timeout=${60}  on_timeout=kill
    Should Be equal As Integers  ${result1.rc}  ${0}
    Should Be equal As Integers  ${result2.rc}  ${0}

CLS Can Scan Special File That Cannot Be Read
    Register Cleanup    Exclude SUSI Illegal seek error
    Register Cleanup    Exclude Failed To Scan Special File That Cannot Be Read In Threat Detector Logs
    Register Cleanup    Run Process  ip  netns  delete  avtest  stderr=STDOUT
    ${result} =  Run Process  ip  netns  add  avtest  stderr=STDOUT
    Register On Fail  Log  ip netns add avtest output is ${result.stdout}
    Should Be equal As Integers  ${result.rc}  ${0}
    Wait Until File exists  /run/netns/avtest
    ${catrc}   ${catoutput} =    Run And Return Rc And Output    cat /run/netns/avtest
    Register On Fail  Log  cat result: = ${catrc} output = ${catoutput}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /run/netns/avtest
    Register On Fail  Log  avscanner output is ${output}
    Dump Log  ${SUSI_DEBUG_LOG_PATH}
    Should Be Equal As Integers  ${rc}  ${ERROR_RESULT}

Threat Detector Client Attempts To Reconnect If AV Plugin Is Not Ready
    register cleanup  Dump Log   ${THREAT_DETECTOR_LOG_PATH}
    Create File     ${NORMAL_DIRECTORY}/eicar_file    ${EICAR_STRING}
    Stop AV Plugin process

    ${av_mark} =  Mark AV Log
    ${td_mark} =  Mark Sophos Threat Detector Log
    ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   ${NORMAL_DIRECTORY}  -x  /mnt/   stdout=${LOG_FILE}   stderr=STDOUT
    wait_for_log_contains_from_mark  ${td_mark}     Detected     timeout=120
    wait_for_log_contains_from_mark  ${td_mark}     ThreatReporterClient failed to connect      timeout=120
    Start AV Plugin Process
    wait_for_log_contains_from_mark  ${td_mark}     ThreatReporterClient connected    timeout=120

    Wait Until AV Plugin Log Contains Detection Name And Path After Mark  ${av_mark}  EICAR-AV-Test  ${NORMAL_DIRECTORY}/eicar_file
    Wait Until AV Plugin Log Contains Detection Event XML After Mark
    ...  mark=${av_mark}
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${NORMAL_DIRECTORY}/eicar_file


AV Scanner waits for threat detector process
    Create File     ${NORMAL_DIRECTORY}/eicar_file    ${EICAR_STRING}

    Stop AV
    register on fail  Start AV

    ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   ${NORMAL_DIRECTORY}   -x   /mnt/   stdout=${LOG_FILE}   stderr=STDOUT

    ## cannot wait for messages in the log as it is buffered and not written syncronously.
    Sleep   10s
    Start AV
    ${result} =  Wait For Process  handle=${HANDLE}  timeout=180s
    Process Should Be Stopped  handle=${HANDLE}

    Dump Log   ${LOG_FILE}
    File Log Contains   ${LOG_FILE}  Failed to connect to Sophos Threat Detector
    Should Be Equal As Integers  ${result.rc}  ${VIRUS_DETECTED_RESULT}


AV Scanner stops promptly while trying to connect
    Create File     ${NORMAL_DIRECTORY}/eicar_file    ${EICAR_STRING}

    Stop AV
    register cleanup  Start AV

    ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   ${NORMAL_DIRECTORY}   -x   /mnt/    /run/snapd/ns/   stdout=${LOG_FILE}   stderr=STDOUT

    ## cannot wait for messages in the log as it is buffered and not written syncronously.
    Sleep   5s
    Send Signal To Process   SIGINT   handle=${HANDLE}
    ${result} =  Wait For Process  handle=${HANDLE}  timeout=5s
    Process Should Be Stopped  handle=${HANDLE}

    Dump Log   ${LOG_FILE}
    File Log Contains   ${LOG_FILE}  Failed to connect to Sophos Threat Detector
    File Log Contains   ${LOG_FILE}  Scan manually aborted
    Should Be Equal As Integers  ${result.rc}  ${1}


AV Scanner stops promptly during a scan
    ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   /  -x  /mnt/  file_samples/  /run/snapd/ns/  stdout=${LOG_FILE}   stderr=STDOUT
    Wait Until File exists  ${LOG_FILE}
    ${cls_mark} =  Mark Log Size  ${LOG_FILE}
    Wait For File With Particular Contents   \ Scanning\   ${LOG_FILE}

    #Stop Threat detector during a scan
    Stop AV
    register cleanup  Start AV

    Send Signal To Process   SIGINT   handle=${HANDLE}
    ${result} =  Wait For Process  handle=${HANDLE}  timeout=5s
    Process Should Be Stopped  handle=${HANDLE}

    Dump Log   ${LOG_FILE}
    #Depending on whether a scan is being processed or it is being requested one of these 2 errors should appear
    ${expected} =  Create List  Failed to receive scan response  Failed to send scan request
    Wait For Log Contains One Of After Mark   ${LOG_FILE}   ${expected}   ${cls_mark}
    Should Be Equal As Integers  ${result.rc}  ${MANUAL_INTERUPTION_RESULT}

# Run this last to reduce effect of AV restart causing TD to be restarted
CLS Aborts Scan If Sophos Threat Detector Is Killed And Does Not Recover
    [Timeout]  15min

    ${LOG_FILE} =          Set Variable   ${NORMAL_DIRECTORY}/scan.log
    ${DETECTOR_BINARY} =   Set Variable   ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector_launcher
    FakeWatchdog.expect_start_failure

    ${HANDLE} =    Start Process    ${CLI_SCANNER_PATH}   /  -x  /mnt/  file_samples/  /run/snapd/ns/  stdout=${LOG_FILE}   stderr=STDOUT
    Register cleanup  dump log  ${LOG_FILE}
    Register Cleanup  Exclude Scan Errors From File Samples
    Register On Fail  Terminate Process  handle=${HANDLE}  kill=True
    # Rename the sophos threat detector launcher so that it cannot be restarted
    Move File  ${DETECTOR_BINARY}  ${DETECTOR_BINARY}_moved
    register cleanup  Start AV
    register cleanup  Stop AV
    register cleanup  Move File  ${DETECTOR_BINARY}_moved  ${DETECTOR_BINARY}

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  File Log Contains  ${LOG_FILE}  Scanning
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -SIGSEGV   ${output}
    sleep  60  Waiting for the socket to timeout
    Wait Until Keyword Succeeds
    ...  ${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD} secs
    ...  10 secs
    ...  File Log Contains  ${LOG_FILE}  Reached total maximum number of reconnection attempts. Aborting scan.

    # After the log message, only wait ten seconds for avscanner to exit
    ${result} =  Wait For Process  handle=${HANDLE}  timeout=10s  on_timeout=kill

    ${line_count} =  Count Lines In Log  ${LOG_FILE}  Failed to scan file

    Should Be True  ${0} < ${line_count} < ${10}

    Check Log Contains String N Times
    ...  ${LOG_FILE}
    ...  scan.log
    ...  Reached total maximum number of reconnection attempts. Aborting scan.
    ...  1

    # Specific codes tested in integration
    Should Be True  ${result.rc} > ${0}

