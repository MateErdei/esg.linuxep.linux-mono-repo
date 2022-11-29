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
Library         ../Libs/FileSampleObfuscator.py

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
    Wait Until SafeStore running

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    File Should Exist    ${SAFESTORE_DB_PASSWORD_PATH}


SafeStore Can Reinitialise Database Containing Threats
    Unpack SafeStore Tools To  ${safestore_tools_unpacked}
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until SafeStore running

    ${ssPassword1} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:

    ${filesInSafeStoreDb1} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb1.stdout}

    Stop SafeStore
    Check Safestore Not Running

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}

    Start SafeStore
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantine Manager initialised OK
    Wait For Log Contains From Mark  ${safestore_mark}  Successfully initialised SafeStore database

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    ${ssPassword2} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    Should Be Equal As Strings    ${ssPassword1}    ${ssPassword2}

    ${filesInSafeStoreDb2} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb2.stdout}

    Should Be Equal    ${filesInSafeStoreDb1.stdout}    ${filesInSafeStoreDb2.stdout}


SafeStore Recovers From Corrupt Database
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until SafeStore running

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Corrupt SafeStore Database

    Check SafeStore Dormant Flag Exists

    Wait For Log Contains From Mark  ${safestore_mark}  Successfully removed corrupt SafeStore database    200
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantine Manager initialised OK

    Check Safestore Dormant Flag Does Not Exist

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${safestore_mark}  Finalised file: ${SCAN_DIRECTORY}/eicar.com

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise


SafeStore Quarantines When It Receives A File To Quarantine
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${av_mark}  Quarantine succeeded
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=e52cf957-a0dc-5b12-bad2-561197a5cae4
    ...  succeeded=1
    ...  origin=1
    ...  result=0
    ...  path=${SCAN_DIRECTORY}/eicar.com


SafeStore Quarantines Archive
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.

    ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
    Create Directory  ${ARCHIVE_DIR}
    Create File  ${ARCHIVE_DIR}/1_dsa    ${DSA_BY_NAME_STRING}
    Create File  ${ARCHIVE_DIR}/2_eicar  ${EICAR_STRING}
    Run Process  tar  --mtime\=UTC 2022-01-01  -C  ${ARCHIVE_DIR}  -cf  ${NORMAL_DIRECTORY}/test.tar  1_dsa  2_eicar
    Remove Directory  ${ARCHIVE_DIR}  recursive=True

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/test.tar  --scan-archives

    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${av_mark}  Quarantine succeeded
    File Should Not Exist   ${SCAN_DIRECTORY}/test.tar

    Wait Until Base Has Core Clean Event
    ...  alert_id=49c016d1-fcfe-543d-8279-6ff8c8f3ce4b
    ...  succeeded=1
    ...  origin=1
    ...  result=0
    ...  path=${SCAN_DIRECTORY}/test.tar


Failed Clean Event Gets Sent When SafeStore Fails To Quarantine A File
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.   timeout=60

    Wait Until SafeStore running
    Remove Directory     ${SAFESTORE_DB_DIR}  recursive=True

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${av_mark}  Quarantine failed
    File Should Exist   ${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=e52cf957-a0dc-5b12-bad2-561197a5cae4
    ...  succeeded=0
    ...  origin=1
    ...  result=3
    ...  path=${SCAN_DIRECTORY}/eicar.com


SafeStore does not quarantine on a Corrupt Database
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.  timeout=60

    Wait Until SafeStore running

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Corrupt SafeStore Database
    Check avscanner can detect eicar

    Wait Until AV Plugin Log Contains Detection Name After Mark  ${av_mark}  EICAR-AV-Test
    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${safestore_mark}  Cannot quarantine file, SafeStore is in
    Wait For Log Contains From Mark  ${safestore_mark}  Successfully removed corrupt SafeStore database    timeout=200
    Wait For Log Contains From Mark  ${safestore_mark}  Successfully initialised SafeStore database
    File Should Exist  ${SCAN_DIRECTORY}/eicar.com

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    # Rescan the file without creating it again
    Check avscanner can detect eicar in  ${SCAN_DIRECTORY}/eicar.com
    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${safestore_mark}  Finalising file
    File Should Not Exist  ${SCAN_DIRECTORY}/eicar.com

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise


With SafeStore Enabled But Not Running We Can Send Threats To AV
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Stop SafeStore

    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.   timeout=60

    Check avscanner can detect eicar

    Wait Until AV Plugin Log Contains Detection Name After Mark  ${av_mark}  EICAR-AV-Test
    Wait For Log Contains From Mark  ${av_mark}    Failed to write to SafeStore socket.
    Check SafeStore Not Running
    Mark Expected Error In Log    ${AV_PLUGIN_PATH}/log/av.log    Aborting SafeStore connection : failed to read length


SafeStore Does Not Attempt To Quarantine File On ReadOnly Mount
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Check avscanner can detect eicar on read only mount

    Wait For Log Contains From Mark  ${av_mark}      File is located on a ReadOnly mount:
    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'


SafeStore Does Not Attempt To Quarantine File On A Network Mount
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.  timeout=60

    Check avscanner can detect eicar on network mount

    Wait For Log Contains From Mark  ${av_mark}      File is located on a Network mount:
    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'


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

    Create Big Eicar Of Size  31K   ${eicar1}
    Create Big Eicar Of Size  31K   ${eicar2}
    Create Big Eicar Of Size  31K   ${eicar3}

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.  timeout=60

    Wait Until SafeStore running

    ${ss_mark} =  Get SafeStore Log Mark
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar1}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${SCAN_DIRECTORY}/${eicar1}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${NORMAL_DIRECTORY}/${eicar2}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar3}

    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${NORMAL_DIRECTORY}/${eicar3}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar3}

    Should Not Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}


SafeStore Purges The Oldest Detection In Its Database When It Exceeds Detection Count
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}

    Stop SafeStore
    Create File     ${COMPONENT_ROOT_PATH}/var/safestore_config.json    { "MaxStoredObjectCount" : 2 }
    Start SafeStore

    ${eicar1}=    Set Variable     eicar1
    ${eicar2}=    Set Variable     eicar2
    ${eicar3}=    Set Variable     eicar3

    # Unique SHAs required to trigger autopurge on object count
    Create File     ${NORMAL_DIRECTORY}/${eicar1}   ${EICAR_STRING} one
    Create File     ${NORMAL_DIRECTORY}/${eicar2}   ${EICAR_STRING} two
    Create File     ${NORMAL_DIRECTORY}/${eicar3}   ${EICAR_STRING} three

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark   ${av_mark}   SafeStore flag set. Setting SafeStore to enabled.   timeout=60

    Wait Until SafeStore running

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar1}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${NORMAL_DIRECTORY}/${eicar1}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${NORMAL_DIRECTORY}/${eicar2}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar3}

    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${NORMAL_DIRECTORY}/${eicar3}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar3}

    Should Not Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}

SafeStore Does Not Restore Quarantined Files When Uninstalled
    [Teardown]  SafeStore Test Teardown When AV Already Uninstalled
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    wait_for_log_contains_from_mark  ${safestore_mark}  Received Threat:
    wait_for_log_contains_from_mark  ${av_mark}  Quarantine succeeded
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

    AV Plugin uninstalls

    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

SafeStore Runs As Root
    Start SafeStore Manually
    ${safestore_pid} =  Get Process Id   handle=${SAFESTORE_HANDLE}
    ${rc}   ${output} =    Run And Return Rc And Output   ps -o user= -p ${safestore_pid}

    Log   ${output}
    Should Contain   ${output}    root

    Stop SafeStore Manually

SafeStore Quarantines File With Same Path And Sha Again And Discards The Previous Object
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Threat ID: e52cf957-a0dc-5b12-bad2-561197a5cae4
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${SCAN_DIRECTORY}/eicar.com successfully
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Threat ID: e52cf957-a0dc-5b12-bad2-561197a5cae4
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${SCAN_DIRECTORY}/eicar.com successfully
    Wait For Log Contains From Mark  ${safestore_mark}  ${SCAN_DIRECTORY}/eicar.com has been quarantined before
    Wait For Log Contains From Mark  ${safestore_mark}  Removing object
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

    # Now try a different path
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Create File  ${NORMAL_DIRECTORY}/eicar2.com  ${EICAR_STRING}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar2.com
    Wait For Log Contains From Mark  ${safestore_mark}  Threat ID: 49f9af79-a8bc-5436-9d3a-404a461a976e
    Wait For Log Contains From Mark  ${av_mark}  Quarantine succeeded
    check_log_does_not_contain_after_mark  ${SAFESTORE_LOG_PATH}  has been quarantined before  ${safestore_mark}
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar2.com


Threat Detector Triggers SafeStore Rescan On Timeout
    ${ss_mark} =  Get SafeStore Log Mark
    Create Rescan Interval File
    Wait For SafeStore Log Contains After Mark  SafeStore Database Rescan request received.  ${ss_mark}


SafeStore Rescan Does Not Restore Or Report Threats
    ## TODO: LINUXDAR-5921 - Replace rescan trigger with policy change to stop this test being flaky.
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}

    ${eicar1}=    Set Variable     eicar1
    ${eicar2}=    Set Variable     eicar2

    # Unique SHAs required to trigger autopurge on object count
    Create File     ${NORMAL_DIRECTORY}/${eicar1}   ${EICAR_STRING} one
    Create File     ${NORMAL_DIRECTORY}/${eicar2}   ${EICAR_STRING} two

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark   ${av_mark}   SafeStore flag set. Setting SafeStore to enabled.   timeout=60

    Wait Until SafeStore running

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar1}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${NORMAL_DIRECTORY}/${eicar1}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Finalised file: ${NORMAL_DIRECTORY}/${eicar2}

    ${filesInSafeStoreDb} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Create Rescan Interval File  30
    Wait For SafeStore Log Contains After Mark  SafeStore Database Rescan request received.  ${ss_mark}   timeout=35
    Wait For SafeStore Log Contains After Mark  Rescan found quarantined file still a threat: ${NORMAL_DIRECTORY}/${eicar1}  ${ss_mark}  timeout=15
    Wait For SafeStore Log Contains After Mark  Rescan found quarantined file still a threat: ${NORMAL_DIRECTORY}/${eicar2}  ${ss_mark}  timeout=15
    Check Log Does Not Contain After Mark  ${AV_LOG_PATH}   Found 'EICAR-AV-Test'   ${av_mark}



Threat Detector Rescan Socket Does Not Block Shutdown
    Stop SafeStore
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Create Rescan Interval File
    wait_for_log_contains_from_mark  ${td_mark}  Failed to connect to SafeStore Rescan - retrying after sleep
    Stop sophos_threat_detector
    wait_for_log_contains_from_mark  ${td_mark}  Stop requested while connecting to SafeStore Rescan


AV Plugin Does Not Quarantine File When SafeStore Is Disabled
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag not set. Setting SafeStore to disabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    Wait For Log Contains From Mark  ${av_mark}  ${SCAN_DIRECTORY}/eicar.com was not quarantined due to SafeStore being disabled

    Wait Until Base Has Core Clean Event
    ...  alert_id=e52cf957-a0dc-5b12-bad2-561197a5cae4
    ...  succeeded=0
    ...  origin=1
    ...  result=3
    ...  path=${SCAN_DIRECTORY}/eicar.com

    File Should Exist   ${SCAN_DIRECTORY}/eicar.com

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}
    ${print_tool_result} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${print_tool_result.stdout}
    Should Not Contain  ${print_tool_result.stdout}  eicar.com


SafeStore Does Not Quarantine Ml Detection By Default
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.
    Wait For Log Contains From Mark  ${av_mark}  SafeStore Quarantine ML flag not set. SafeStore will not quarantine ML detections.

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    Wait For Log Contains From Mark  ${av_mark}  ${NORMAL_DIRECTORY}/MLengHighScore.exe was not quarantined due to being reported as an ML detection

    Wait Until Base Has Core Clean Event
    ...  alert_id=88b2d8f4-6a29-5e0b-8e01-daf1695a61e9
    ...  succeeded=0
    ...  origin=0
    ...  result=3
    ...  path=${NORMAL_DIRECTORY}/MLengHighScore.exe

    File Should Exist  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}
    ${print_tool_result} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${print_tool_result.stdout}
    Should Not Contain  ${print_tool_result.stdout}  MLengHighScore.exe


SafeStore Quarantines Ml Detection If Ml Flag Is Enabled
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_quarantine_ml_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.
    Wait For Log Contains From Mark  ${av_mark}  SafeStore Quarantine ML flag set. SafeStore will quarantine ML detections.

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    Wait For Log Contains From Mark  ${safestore_mark}  Finalised file: ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Wait For Log Contains From Mark  ${av_mark}  Quarantine succeeded

    File Should Not Exist  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    Wait Until Base Has Core Clean Event
    ...  alert_id=88b2d8f4-6a29-5e0b-8e01-daf1695a61e9
    ...  succeeded=1
    ...  origin=0
    ...  result=0
    ...  path=${NORMAL_DIRECTORY}/MLengHighScore.exe

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}
    ${print_tool_result} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${print_tool_result.stdout}
    Should Contain  ${print_tool_result.stdout}  MLengHighScore.exe


SafeStore Does Not Quarantine Pua Detection
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/PsExec.exe  ${NORMAL_DIRECTORY}/PsExec.exe

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/PsExec.exe

    Wait For Log Contains From Mark  ${av_mark}  ${NORMAL_DIRECTORY}/PsExec.exe was not quarantined due to being a PUA

    Wait Until Base Has Core Clean Event
    ...  alert_id=ca835a27-afe8-5e24-888c-5284c0ea4ac8
    ...  succeeded=0
    ...  origin=3
    ...  result=3
    ...  path=${NORMAL_DIRECTORY}/PsExec.exe

    File Should Exist  ${NORMAL_DIRECTORY}/PsExec.exe

    Unpack SafeStore Tools To  ${safestore_tools_unpacked}
    ${print_tool_result} =  Run Process  ${safestore_tools_unpacked}/tap_test_output/safestore_print_tool
    Log  ${print_tool_result.stdout}
    Should Not Contain  ${print_tool_result.stdout}  PsExec.exe


*** Keywords ***
SafeStore Test Setup
    Require Plugin Installed and Running  DEBUG
    LogUtils.save_log_marks_at_start_of_test
    Start SafeStore
    Wait Until SafeStore Started Successfully

    Set Suite Variable  ${safestore_tools_unpacked}  /tmp/safestoretools/tap_test_output
    Create Directory  ${NORMAL_DIRECTORY}

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
    Stop SafeStore
    run cleanup functions
    run failure functions if failed
    dump log  ${SAFESTORE_LOG_PATH}
    dump log  ${WATCHDOG_LOG}
    dump log  ${SOPHOS_INSTALL}/plugins/av/log/av.log
    List Directory  ${SOPHOS_INSTALL}/plugins/av/var
    Remove Directory    ${SAFESTORE_DB_DIR}  recursive=True
    Remove Directory    ${NORMAL_DIRECTORY}  recursive=True

    #restore machineID file
    Create File  ${MACHINEID_FILE}  3ccfaf097584e65c6c725c6827e186bb
    Remove File  ${CUSTOMERID_FILE}

    run keyword if test failed  Restart AV Plugin And Clear The Logs For Integration Tests

SafeStore Test Teardown When AV Already Uninstalled
    run cleanup functions
    run failure functions if failed

    Remove Directory    ${NORMAL_DIRECTORY}  recursive=True

    #restore machineID file
    Create File  ${MACHINEID_FILE}  3ccfaf097584e65c6c725c6827e186bb
    Remove File  ${CUSTOMERID_FILE}

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

Create Big Eicar Of Size
    [Arguments]  ${size}  ${filename}
    Create File     ${NORMAL_DIRECTORY}/${filename}    ${EICAR_STRING}
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/${filename}
    ${result} =   Run Process   head  -c  ${size}  </dev/urandom  >>${NORMAL_DIRECTORY}/${filename}  shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}

Create Rescan Interval File
    [Arguments]  ${interval}=1
    Stop Sophos Threat Detector
    Create File    ${SOPHOS_INSTALL}/plugins/av/chroot/var/safeStoreRescanInterval    ${interval}
    Register Cleanup   Remove File  ${SOPHOS_INSTALL}/plugins/av/chroot/var/safeStoreRescanInterval
    Start Sophos Threat Detector

    ## Short interval can leak between tests, so have to restart TD to clear it from the process.
    Register Cleanup   Stop Sophos Threat Detector
    Register Cleanup   Mark Expected Error In Log    ${THREAT_DETECTOR_LOG_PATH}    UnixSocket <> Aborting scan, scanner is shutting down
    Register Cleanup   Start Sophos Threat Detector
