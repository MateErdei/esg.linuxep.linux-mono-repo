*** Settings ***
Documentation    Product tests of SafeStore
Force Tags       INTEGRATION  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot

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
${SAFESTORE_DB_DIR}                  ${SOPHOS_INSTALL}/plugins/av/var/safestore_db
${SAFESTORE_DB_PATH}                 ${SAFESTORE_DB_DIR}/safestore.db
${SAFESTORE_DB_PASSWORD_PATH}        ${SAFESTORE_DB_DIR}/safestore.pw
${SAFESTORE_DORMANT_FLAG}            ${SOPHOS_INSTALL}/plugins/av/var/safestore_dormant_flag


*** Test Cases ***

SafeStore Database is Initialised
    register on fail  dump log  ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    register on fail  dump log  ${SOPHOS_INSTALL}/base/etc/logger.conf
    wait for Safestore to be running

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    File Should Exist    ${SAFESTORE_DB_PASSWORD_PATH}

SafeStore Can Reinitialise Database Containing Threats
    Unpack SafeStore Tools To  ${safestore_tools_unpacked}
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    wait for Safestore to be running

    ${ssPassword1} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:

    ${filesInSafeStoreDb1} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb1.stdout}

    Stop SafeStore
    Check Safestore Not Running
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}

    Start SafeStore
    wait_for_log_contains_from_mark  ${safestore_mark}  Quarantine Manager initialised OK
    wait_for_log_contains_from_mark  ${safestore_mark}  Successfully initialised SafeStore database

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    ${ssPassword2} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    Should Be Equal As Strings    ${ssPassword1}    ${ssPassword2}

    ${filesInSafeStoreDb2} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb2.stdout}

    Should Be Equal    ${filesInSafeStoreDb1.stdout}    ${filesInSafeStoreDb2.stdout}

SafeStore Recovers From Corrupt Database
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    wait for Safestore to be running

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Corrupt SafeStore Database

    Check SafeStore Dormant Flag Exists

    wait_for_log_contains_from_mark  ${safestore_mark}  Successfully removed corrupt SafeStore database    200
    wait_for_log_contains_from_mark  ${safestore_mark}  Quarantine Manager initialised OK

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
    ...  origin=0
    ...  result=0
    ...  path=${SCAN_DIRECTORY}/eicar.com


Failed Clean Event Gets Sent When SafeStore Fails To Quarantine A File
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}   timeout=60

    Wait for Safestore to be running
    Remove Directory     ${SAFESTORE_DB_DIR}  recursive=True

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${av_mark}  Quarantine failed
    File Should Exist   ${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=Tbd7be297ddf3cd8
    ...  succeeded=0
    ...  origin=0
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

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:

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


SafeStore Purges The Oldest Detection In Its Database When It Exceeds Storage Capacity
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}

    Stop SafeStore
    # The MaxSafeStoreSize could cause flakiness in the future if the footprint of the SafeStore instance grows, which we can't avoid (fix by increasing its size further)
    Create File     ${COMPONENT_ROOT_PATH}/var/safestore_config.json    { "MaxObjectSize" : 32000, "MaxSafeStoreSize" : 144000 }
    Start SafeStore

    ${eicar1}=    Set Variable     big_eicar1
    ${eicar2}=    Set Variable     big_eicar2
    ${eicar3}=    Set Variable     big_eicar3

    Create Big Eicar   ${eicar1}
    Create Big Eicar   ${eicar2}
    Create Big Eicar   ${eicar3}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}  timeout=60

    Wait For Safestore To Be Running

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar1}

    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${eicar1}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${eicar2}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar3}

    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${eicar3}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar3}

    Should Not Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}


SafeStore Purges The Oldest Detection In Its Database When It Exceeds Detection Count
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}

    Stop SafeStore
    Create File     ${COMPONENT_ROOT_PATH}/var/safestore_config.json    { "MaxRegObjectCount" : 1, "MaxStoreObjectCount" : 2 }
    Start SafeStore

    ${eicar1}=    Set Variable     eicar1
    ${eicar2}=    Set Variable     eicar2
    ${eicar3}=    Set Variable     eicar3

    Create File     ${NORMAL_DIRECTORY}/${eicar1}   ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/${eicar2}   ${EICAR_STRING}
    Create File     ${NORMAL_DIRECTORY}/${eicar3}   ${EICAR_STRING}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}  timeout=60

    Wait For Safestore To Be Running

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar1}

    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${eicar1}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${eicar2}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar3}

    Wait For AV Log Contains After Mark    Found 'EICAR-AV-Test'  ${av_mark}
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${eicar3}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar3}

    Should Not Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}


*** Keywords ***
SafeStore Test Setup
    Require Plugin Installed and Running  DEBUG
    LogUtils.save_log_marks_at_start_of_test
    Start SafeStore
    Wait For Safestore To Be Running

    Set Suite Variable  ${safestore_tools_unpacked}  /tmp/safestoretools/tap_test_output

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

    Stop SafeStore
    Remove Directory    ${SAFESTORE_DB_DIR}  recursive=True

    #restore machineID file
    Create File  ${MACHINEID_FILE}  3ccfaf097584e65c6c725c6827e186bb
    Remove File  ${CUSTOMERID_FILE}

    Empty Directory  ${NORMAL_DIRECTORY}

    run keyword if test failed  Restart AV Plugin And Clear The Logs For Integration Tests

Corrupt SafeStore Database
    Stop SafeStore
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/persist-safeStoreDbErrorThreshold    1

    Remove Files    ${SAFESTORE_DB_PATH}    ${SAFESTORE_DB_PASSWORD_PATH}
    Copy Files    ${RESOURCES_PATH}/safestore_db_corrupt/*    ${SAFESTORE_DB_DIR}
    Start SafeStore

Check SafeStore Dormant Flag Exists
    [Arguments]  ${timeout}=15  ${interval}=2
    Wait Until File exists  ${SAFESTORE_DORMANT_FLAG}  ${timeout}  ${interval}

Check Safestore Dormant Flag Does Not Exist
    File Should Not Exist  ${SAFESTORE_DORMANT_FLAG}

Create Big Eicar
    [Arguments]  ${filename}
    Create File     ${NORMAL_DIRECTORY}/${filename}    ${EICAR_STRING}
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/${filename}
    ${result} =   Run Process   head  -c  31K  </dev/urandom  >>${NORMAL_DIRECTORY}/${filename}  shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}
