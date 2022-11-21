*** Settings ***
Documentation    Product tests of SafeStore
Force Tags       INTEGRATION  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/SafeStoreResources.robot
Resource    ../shared/OnAccessResources.robot

Library         ../Libs/CoreDumps.py
Library         ../Libs/OnFail.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ProcessUtils.py

Library         OperatingSystem
Library         Collections

Test Setup      SafeStore Test Setup
Test Teardown   SafeStore Test TearDown

*** Variables ***
${CLEAN_STRING}                      not an eicar
${NORMAL_DIRECTORY}                  /home/vagrant/this/is/a/directory/for/scanning
${CUSTOMERID_FILE}                   ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/var/customer_id.txt
${MACHINEID_CHROOT_FILE}             ${COMPONENT_ROOT_PATH}/chroot${SOPHOS_INSTALL}/base/etc/machine_id.txt
${MACHINEID_FILE}                    ${SOPHOS_INSTALL}/base/etc/machine_id.txt
${SAFESTORE_DORMANT_FLAG}            ${SOPHOS_INSTALL}/plugins/av/var/safestore_dormant_flag


*** Test Cases ***

SafeStore Database is Initialised
    register on fail  dump log  ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    register on fail  dump log  ${SOPHOS_INSTALL}/base/etc/logger.conf
    wait for Safestore to be running

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    File Should Exist    ${SAFESTORE_DB_PASSWORD_PATH}

SafeStore Can Reinitialise Database Containing Threats
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    wait for Safestore to be running

    ${ssPassword1} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:

    ${filesInSafeStoreDb1} =  List Files In Directory  ${SAFESTORE_DB_DIR}
    Log  ${filesInSafeStoreDb1}

    Stop SafeStore
    Check Safestore Not Running

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}

    Start SafeStore
    wait_for_log_contains_from_mark  ${safestore_mark}  Quarantine Manager initialised OK
    wait_for_log_contains_from_mark  ${safestore_mark}  Successfully initialised SafeStore database

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    ${ssPassword2} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    Should Be Equal As Strings    ${ssPassword1}    ${ssPassword2}

    ${filesInSafeStoreDb2} =  List Files In Directory  ${SAFESTORE_DB_DIR}
    Log  ${filesInSafeStoreDb2}

    # Removing tmp file: https://www.sqlite.org/tempfiles.html
    Remove Values From List    ${filesInSafeStoreDb1}    safestore.db-journal
    Should Be Equal    ${filesInSafeStoreDb1}    ${filesInSafeStoreDb2}

SafeStore Recovers From Corrupt Database
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    wait for Safestore to be running

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Corrupt SafeStore Database

    Check SafeStore Dormant Flag Exists

    wait_for_log_contains_from_mark  ${safestore_mark}  Successfully removed corrupt SafeStore database    200
    wait_for_log_contains_from_mark  ${safestore_mark}  Successfully initialised SafeStore database

    Check Safestore Dormant Flag Does Not Exist

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${safestore_mark}  Finalised file: eicar.com

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise

SafeStore Quarantines When It Receives A File To Quarantine
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${av_mark}  Quarantine succeeded
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=Tbd7be297ddf3cd8
    ...  succeeded=1
    ...  origin=1
    ...  result=0
    ...  path=${SCAN_DIRECTORY}/eicar.com


SafeStore Quarantines Archive
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.

    ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
    Create Directory  ${ARCHIVE_DIR}
    Create File  ${ARCHIVE_DIR}/1_dsa    ${DSA_BY_NAME_STRING}
    Create File  ${ARCHIVE_DIR}/2_eicar  ${EICAR_STRING}
    Run Process  tar  -C  ${ARCHIVE_DIR}  -cf  ${NORMAL_DIRECTORY}/test.tar  1_dsa  2_eicar

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/test.tar  --scan-archives

    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${av_mark}  Quarantine succeeded
    File Should Not Exist   ${SCAN_DIRECTORY}/test.tar

    Wait Until Base Has Core Clean Event
    ...  alert_id=Te6df7ee25b75923
    ...  succeeded=1
    ...  origin=1
    ...  result=0
    ...  path=${SCAN_DIRECTORY}/test.tar


Failed Clean Event Gets Sent When SafeStore Fails To Quarantine A File
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}   timeout=60
    Remove Directory     ${SAFESTORE_DB_DIR}  recursive=True


    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${av_mark}  Quarantine failed
    File Should Exist   ${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=Tbd7be297ddf3cd8
    ...  succeeded=0
    ...  origin=1
    ...  result=3
    ...  path=${SCAN_DIRECTORY}/eicar.com


SafeStore does not quarantine on a Corrupt Database
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}  timeout=60

    wait for Safestore to be running

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Corrupt SafeStore Database
    Check avscanner can detect eicar

    Wait Until AV Plugin Log Contains Detection Name After Mark  ${av_mark}  EICAR-AV-Test
    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${safestore_mark}  Cannot quarantine file, SafeStore is in
    wait_for_log_contains_from_mark  ${safestore_mark}  Successfully removed corrupt SafeStore database    timeout=200
    wait_for_log_contains_from_mark  ${safestore_mark}  Successfully initialised SafeStore database
    File Should Exist  ${SCAN_DIRECTORY}/eicar.com

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    # Rescan the file without creating it again
    Check avscanner can detect eicar in  ${SCAN_DIRECTORY}/eicar.com
    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${safestore_mark}  Finalising file
    File Should Not Exist  ${SCAN_DIRECTORY}/eicar.com

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise

With SafeStore Enabled But Not Running We Can Send Threats To AV
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Stop SafeStore

    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}  timeout=60

    Check avscanner can detect eicar

    Wait Until AV Plugin Log Contains Detection Name After Mark  ${av_mark}  EICAR-AV-Test
    Wait For AV Log Contains After Mark  Failed to write to SafeStore socket.  mark=${av_mark}
    Check SafeStore Not Running
    Mark Expected Error In Log    ${AV_PLUGIN_PATH}/log/av.log    Aborting SafeStore connection : failed to read length


SafeStore Does Not Attempt To Quarantine File On ReadOnly Mount
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}  timeout=60

    Check avscanner can detect eicar on read only mount

    Wait For AV Log Contains After Mark    File is located on a ReadOnly mount:  ${av_mark}
    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}


SafeStore Does Not Attempt To Quarantine File On A Network Mount
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}  timeout=60

    Check avscanner can detect eicar on network mount

    Wait For AV Log Contains After Mark    File is located on a Network mount:  ${av_mark}
    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}

SafeStore Does Not Restore Quarantined Files When Uninstalled
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${av_mark}  Quarantine succeeded
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

    Uninstall All

    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

SafeStore Runs As Root
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Create File     ${SCAN_DIRECTORY}/eicar.com  ${EICAR_STRING}
    Change Owner  ${SCAN_DIRECTORY}/eicar.com  root  root
    Register Cleanup   Remove File   ${SCAN_DIRECTORY}/eicar.com

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar in  ${SCAN_DIRECTORY}/eicar.com

    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${av_mark}  Quarantine succeeded

*** Keywords ***
SafeStore Test Setup
    Require Plugin Installed and Running  DEBUG
    LogUtils.save_log_marks_at_start_of_test
    Start SafeStore

    Get AV Log Mark
    Mark Sophos Threat Detector Log

    register on fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register on fail  dump log  ${WATCHDOG_LOG}
    register on fail  dump log  ${SAFESTORE_LOG_PATH}
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

SafeStore Test TearDown
    run cleanup functions
    run failure functions if failed

    #restore machineID file
    Create File  ${MACHINEID_FILE}  3ccfaf097584e65c6c725c6827e186bb
    Remove File  ${CUSTOMERID_FILE}

    run keyword if test failed  Restart AV Plugin And Clear The Logs For Integration Tests

Check SafeStore Dormant Flag Exists
    [Arguments]  ${timeout}=15  ${interval}=2
    Wait Until File exists  ${SAFESTORE_DORMANT_FLAG}  ${timeout}  ${interval}

Check Safestore Dormant Flag Does Not Exist
    File Should Not Exist  ${SAFESTORE_DORMANT_FLAG}


wait for Safestore to be running
    ## password creation only done on first run - can't cover complete log turn-over:
    Wait_For_Entire_log_contains  ${SAFESTORE_LOG_PATH}  Successfully saved SafeStore database password to file  timeout=15

    ## Lines logged for every start
    Wait_For_Log_contains_after_last_restart  ${SAFESTORE_LOG_PATH}  Quarantine Manager initialised OK  timeout=15
    Wait_For_Log_contains_after_last_restart  ${SAFESTORE_LOG_PATH}  Successfully initialised SafeStore database  timeout=5
    Wait_For_Log_contains_after_last_restart  ${SAFESTORE_LOG_PATH}  safestore <> SafeStore started  timeout=5