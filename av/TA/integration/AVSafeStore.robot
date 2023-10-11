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
Library         ../Libs/FileSampleObfuscator.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/ProcessUtils.py
Library         ../Libs/SafeStoreUtils.py

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
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until SafeStore running

    ${ssPassword1} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:

    ${filesInSafeStoreDb1} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
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

    ${filesInSafeStoreDb2} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
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
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${SCAN_DIRECTORY}/eicar.com successfully

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise


SafeStore Recovers From Corrupt Database With Lock Dir
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until SafeStore running
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}

    # Corrupt DB and also create the safestore lock dir
    Corrupt SafeStore Database With Lock

    Check SafeStore Dormant Flag Exists
    Wait For Log Contains From Mark  ${safestore_mark}  Successfully removed corrupt SafeStore database    200
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantine Manager initialised OK

    Check Safestore Dormant Flag Does Not Exist

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${SCAN_DIRECTORY}/eicar.com successfully

    # Internal error due to the lock dir being put into place
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: INTERNAL_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise


SafeStore Recovers From Database With Erroneous Lock Dir
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until SafeStore running
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}

    # Create the SafeStore lock dir and set the number of failures needed to be 1
    Stop SafeStore
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/persist-safeStoreDbErrorThreshold    1
    Create Directory   ${SAFESTORE_DB_DIR}/safestore.db.lock
    Start SafeStore
    Wait Until SafeStore running

    Wait For Log Contains From Mark  ${safestore_mark}  Quarantine Manager initialised OK  200

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${SCAN_DIRECTORY}/eicar.com successfully

    # Internal error due to the lock dir being put into place
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: INTERNAL_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise



SafeStore Quarantines When It Receives A File To Quarantine
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${av_mark}  Threat cleaned up at path:
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
    ...  succeeded=1
    ...  origin=1
    ...  result=0
    ...  path=${SCAN_DIRECTORY}/eicar.com

SafeStore Quarantines When It Receives A File To Quarantine (On Access)
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Start On Access And SafeStore
    ${av_mark} =  Get AV Log Mark
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    On-access Scan Eicar Close
    Exclued SafeStore File Open Error On Quarantine  /tmp_test/eicar.com

    wait_for_log_contains_from_mark  ${safestore_mark}   Quarantined
    wait_for_log_contains_from_mark  ${av_mark}  Threat cleaned up at path:
    File Should Not Exist   /tmp_test/eicar.com
    File Should Not Exist  ${AV_PLUGIN_PATH}/var/onaccess_unhealthy_flag

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=/tmp_test/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
    ...  succeeded=1
    ...  origin=1
    ...  result=0
    ...  path=/tmp_test/eicar.com

SafeStore Quarantines Archive
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.

    ${ARCHIVE_DIR} =  Set Variable  ${NORMAL_DIRECTORY}/archive_dir
    Create Directory  ${ARCHIVE_DIR}
    Create File  ${ARCHIVE_DIR}/1_dsa    ${DSA_BY_NAME_STRING}
    Create File  ${ARCHIVE_DIR}/2_eicar  ${EICAR_STRING}
    Run Process  tar  --mtime\=UTC 2022-01-01  -C  ${ARCHIVE_DIR}  -cf  ${NORMAL_DIRECTORY}/test.tar  1_dsa  2_eicar
    ${archive_sha} =  Get SHA256  ${NORMAL_DIRECTORY}/test.tar
    Remove Directory  ${ARCHIVE_DIR}  recursive=True

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/test.tar  --scan-archives

    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${av_mark}  Threat cleaned up at path:
    File Should Not Exist   ${SCAN_DIRECTORY}/test.tar

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=Troj/TestSFS-G
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=${archive_sha}
    ...  path=${SCAN_DIRECTORY}/test.tar

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
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
    Exclued SafeStore Internal Error On Quarantine   ${SCAN_DIRECTORY}/eicar.com

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    Wait For Log Contains From Mark  ${safestore_mark}  Received Threat:
    Wait For Log Contains From Mark  ${av_mark}  Quarantine failed for threat:  timeout=15
    File Should Exist   ${SCAN_DIRECTORY}/eicar.com

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
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
    Wait For Log Contains From Mark  ${safestore_mark}  Cannot quarantine ${SCAN_DIRECTORY}/eicar.com, SafeStore is in CORRUPT state
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
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Cannot quarantine ${SCAN_DIRECTORY}/eicar.com, SafeStore is in CORRUPT state


With SafeStore Enabled But Not Running We Can Send Threats To AV
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Stop SafeStore

    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.   timeout=60

    Check avscanner can detect eicar

    Wait Until AV Plugin Log Contains Detection Name After Mark  ${av_mark}  EICAR-AV-Test
    Wait For Log Contains From Mark  ${av_mark}    Failed to connect to SafeStore
    Check SafeStore Not Running


SafeStore Does Not Attempt To Quarantine File On ReadOnly Mount
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Check avscanner can detect eicar on read only mount

    Wait For Log Contains From Mark  ${av_mark}      is located on a ReadOnly mount:
    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'


SafeStore Does Not Attempt To Quarantine File On ReadOnly Mount (On Access)
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Create eicar on read only mount  /tmp_test/

    Start On Access And SafeStore
    ${av_mark} =  Get AV Log Mark

    ${result} =  run process    cat   /tmp_test/readOnly/eicar.com

    Log  ${result}
    Wait For Log Contains From Mark  ${av_mark}      is located on a ReadOnly mount:
    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'

SafeStore Does Not Attempt To Quarantine File On A Network Mount
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}      SafeStore flag set. Setting SafeStore to enabled.  timeout=60

    Check avscanner can detect eicar on network mount

    Wait For Log Contains From Mark  ${av_mark}      is located on a Network mount:
    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'


SafeStore Does Not Attempt To Quarantine File On A Network Mount (On Access)
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Create eicar on network mount
    ${destination} =  Set Variable  /testmnt/nfsshare

    Start On Access And SafeStore
    ${av_mark} =  Get AV Log Mark

    ${result} =  run process    cat   ${destination}/eicar.com

    Log  ${result}
    Wait For Log Contains From Mark  ${av_mark}      is located on a Network mount:
    Wait For Log Contains From Mark  ${av_mark}      Found 'EICAR-AV-Test'


SafeStore Purges The Oldest Detection In Its Database When It Exceeds Storage Capacity
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    ${av_mark} =  Get AV Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.  timeout=60

    Wait Until SafeStore running

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

    ${ss_mark} =  Get SafeStore Log Mark
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar1}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${SCAN_DIRECTORY}/${eicar1} successfully

    ${filesInSafeStoreDb} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${NORMAL_DIRECTORY}/${eicar2} successfully

    ${filesInSafeStoreDb} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar3}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${NORMAL_DIRECTORY}/${eicar3} successfully

    ${filesInSafeStoreDb} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar3}

    Should Not Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}


SafeStore Purges The Oldest Detection In Its Database When It Exceeds Detection Count
    register cleanup    Exclude Watchdog Log Unable To Open File Error

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
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${NORMAL_DIRECTORY}/${eicar1} successfully

    ${filesInSafeStoreDb} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${NORMAL_DIRECTORY}/${eicar2} successfully

    ${filesInSafeStoreDb} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar3}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${NORMAL_DIRECTORY}/${eicar3} successfully

    ${filesInSafeStoreDb} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
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
    wait_for_log_contains_from_mark  ${av_mark}  Threat cleaned up at path:
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
    Wait For Log Contains From Mark  ${av_mark}  Threat cleaned up at path:
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar
    Wait For Log Contains From Mark  ${safestore_mark}  Threat ID: e52cf957-a0dc-5b12-bad2-561197a5cae4
    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${SCAN_DIRECTORY}/eicar.com successfully
    Wait For Log Contains From Mark  ${safestore_mark}  ${SCAN_DIRECTORY}/eicar.com has been quarantined before
    Wait For Log Contains From Mark  ${safestore_mark}  Removing object
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com
    Wait For Log Contains From Mark  ${av_mark}  Threat cleaned up at path:

    # Now try a different path
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Create File  ${NORMAL_DIRECTORY}/eicar2.com  ${EICAR_STRING}
    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/eicar2.com
    Wait For Log Contains From Mark  ${safestore_mark}  Threat ID: 49f9af79-a8bc-5436-9d3a-404a461a976e
    Wait For Log Contains From Mark  ${av_mark}  Threat cleaned up at path:
    check_log_does_not_contain_after_mark  ${SAFESTORE_LOG_PATH}  has been quarantined before  ${safestore_mark}
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar2.com

Threat Detector Triggers SafeStore Rescan On Timeout
    ${ss_mark} =  Get SafeStore Log Mark
    Create Rescan Interval File
    Wait For SafeStore Log Contains After Mark  SafeStore Database Rescan request received.  ${ss_mark}


SafeStore Rescan Does Not Restore Or Report Threats
    register cleanup    Exclude Watchdog Log Unable To Open File Error

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
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${NORMAL_DIRECTORY}/${eicar1} successfully

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark

    Check avscanner can detect eicar in  ${NORMAL_DIRECTORY}/${eicar2}

    Wait For Log Contains From Mark  ${av_mark}  Found 'EICAR-AV-Test'
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${NORMAL_DIRECTORY}/${eicar2} successfully

    ${filesInSafeStoreDb} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${filesInSafeStoreDb.stdout}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar1}
    Should Contain  ${filesInSafeStoreDb.stdout}  ${eicar2}

    ${av_mark} =  Get AV Log Mark
    ${ss_mark} =  Get SafeStore Log Mark
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}

    # Trigger Rescan
    Send CORC Policy To Base  corc_policy.xml

    Wait For Log Contains From Mark  ${av_mark}  Added SHA256 to allow list: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    Wait For Log Contains From Mark  ${td_mark}  Triggering rescan of SafeStore database

    Wait For SafeStore Log Contains After Mark  SafeStore Database Rescan request received.  ${ss_mark}   timeout=10
    Wait For SafeStore Log Contains After Mark  Rescan found quarantined file still a threat: ${NORMAL_DIRECTORY}/${eicar1}  ${ss_mark}  timeout=5
    Wait For SafeStore Log Contains After Mark  Rescan found quarantined file still a threat: ${NORMAL_DIRECTORY}/${eicar2}  ${ss_mark}  timeout=5
    Check Log Does Not Contain After Mark  ${AV_LOG_PATH}   Found 'EICAR-AV-Test'   ${av_mark}


Threat Detector Rescan Socket Does Not Block Shutdown
    Stop SafeStore
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Create Rescan Interval File
    wait_for_log_contains_from_mark  ${td_mark}  SafeStoreRescanClient failed to connect
    Stop sophos_threat_detector
    wait_for_log_contains_from_mark  ${td_mark}  SafeStoreRescanClient received stop request while connecting


Allow Listed Files Are Removed From Quarantine
    Wait Until threat detector running
    Wait Until SafeStore running

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${ss_mark} =  Get SafeStore Log Mark

    Send Flags Policy To Base  flags_policy/flags_safestore_quarantine_ml_enabled.json
    Wait For Log Contains From Mark   ${av_mark}   SafeStore flag set. Setting SafeStore to enabled.   timeout=60

    # Create threat to scan
    ${allow_listed_threat_file} =  Set Variable  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${allow_listed_threat_file}
    Register Cleanup  Remove File  ${allow_listed_threat_file}
    Should Exist  ${allow_listed_threat_file}

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${allow_listed_threat_file} successfully
    Should Not Exist  ${allow_listed_threat_file}

    ${ss_mark} =  Get SafeStore Log Mark

    # Allow-list the file
    Send CORC Policy To Base  corc_policy.xml

    wait_for_log_contains_from_mark  ${av_mark}  Added SHA256 to allow list: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    wait_for_log_contains_from_mark  ${td_mark}  Triggering rescan of SafeStore database
    Wait For Log Contains From Mark  ${ss_mark}  SafeStore Database Rescan request received

    wait_for_log_contains_from_mark  ${td_mark}  Allowed by SHA256: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    Wait For Log Contains From Mark  ${ss_mark}  Restored file to disk: ${allow_listed_threat_file}

    # File allowed so should still exist
    Should Exist  ${allow_listed_threat_file}

    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}

    # Scan threat
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    # File is allowed and not treated as a threat
    wait_for_log_contains_from_mark  ${td_mark}  Allowed by SHA256: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9

    # File allowed so should still exist
    Should Exist  ${allow_listed_threat_file}


AV Plugin Does Not Quarantine File When SafeStore Is Disabled
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag not set. Setting SafeStore to disabled.    timeout=60

    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Check avscanner can detect eicar

    Wait For Log Contains From Mark  ${av_mark}  ${SCAN_DIRECTORY}/eicar.com was not quarantined due to SafeStore being disabled

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
    ...  succeeded=0
    ...  origin=1
    ...  result=3
    ...  path=${SCAN_DIRECTORY}/eicar.com

    File Should Exist   ${SCAN_DIRECTORY}/eicar.com

    ${print_tool_result} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
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

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=ML/PE-A
    ...  threat_type=1
    ...  origin=0
    ...  remote=false
    ...  sha256=c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    ...  path=${NORMAL_DIRECTORY}/MLengHighScore.exe

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
    ...  succeeded=0
    ...  origin=0
    ...  result=3
    ...  path=${NORMAL_DIRECTORY}/MLengHighScore.exe

    File Should Exist  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    ${print_tool_result} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
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

    Wait For Log Contains From Mark  ${safestore_mark}  Quarantined ${NORMAL_DIRECTORY}/MLengHighScore.exe successfully
    Wait For Log Contains From Mark  ${av_mark}  Threat cleaned up at path:

    File Should Not Exist  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=ML/PE-A
    ...  threat_type=1
    ...  origin=0
    ...  remote=false
    ...  sha256=c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    ...  path=${NORMAL_DIRECTORY}/MLengHighScore.exe

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
    ...  succeeded=1
    ...  origin=0
    ...  result=0
    ...  path=${NORMAL_DIRECTORY}/MLengHighScore.exe

    ${print_tool_result} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${print_tool_result.stdout}
    Should Contain  ${print_tool_result.stdout}  MLengHighScore.exe

Safestore Quarantines On Access Execute Detection
    ${oa_mark} =  get_on_access_log_mark
    ${ss_mark} =  Get SafeStore Log Mark

    Send Policies to enable on-access  flags_policy/flags_safestore_onaccess_and_ml_enabled.json  ${oa_mark}
    On-access Scan On Execute   ${TRUE}     ${ss_mark}

SafeStore Quarantines Pua Detection
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/PsExec.exe  ${NORMAL_DIRECTORY}/PsExec.exe

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    ${safestore_mark} =  mark_log_size  ${SAFESTORE_LOG_PATH}
    Run Process  ${CLI_SCANNER_PATH}  ${NORMAL_DIRECTORY}/PsExec.exe

    Check Log Does Not Contain After Mark  ${AV_LOG_PATH}  ${NORMAL_DIRECTORY}/PsExec.exe was not quarantined due to being a PUA    ${av_mark}

    ${correlation_id} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=PsExec
    ...  threat_type=2
    ...  origin=3
    ...  remote=false
    ...  sha256=3337e3875b05e0bfba69ab926532e3f179e8cfbf162ebb60ce58a0281437a7ef
    ...  path=${NORMAL_DIRECTORY}/PsExec.exe

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id}
    ...  succeeded=1
    ...  origin=3
    ...  result=0
    ...  path=${NORMAL_DIRECTORY}/PsExec.exe

    File Should Not Exist  ${NORMAL_DIRECTORY}/PsExec.exe

    ${print_tool_result} =  Run Process  ${AV_TEST_TOOLS}/safestore_print_tool
    Log  ${print_tool_result.stdout}
    Should Contain  ${print_tool_result.stdout}  PsExec.exe

Threat Can Be Restored From Persisted SafeStore Database
    Check AV Plugin Running
    ${avMark} =  Get AV Log Mark
    ${safeStoreMark} =  Mark Log Size  ${SAFESTORE_LOG_PATH}
    ${threatId} =  Set Variable    e52cf957-a0dc-5b12-bad2-561197a5cae4

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${avMark}      SafeStore flag set. Setting SafeStore to enabled.  timeout=60

    Check avscanner can detect eicar

    Wait For Log Contains From Mark  ${safeStoreMark}  Threat ID: ${threatId}
    Wait For Log Contains From Mark  ${avMark}  Threat cleaned up at path:
    File Should Not Exist    ${SCAN_DIRECTORY}/eicar.com

    ${dbContent} =    Get Contents of SafeStore Database

    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed

    Install AV Directly from SDDS
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    File Log Contains    ${AV_INSTALL_LOG}    Successfully restored old SafeStore database
    Verify SafeStore Database Exists
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

    ${dbContentBeforeRestore} =    Get Contents of SafeStore Database
    Should Contain    ${dbContentBeforeRestore}    ${threatId}
    Should Be Equal    ${dbContent}    ${dbContentBeforeRestore}
    File Should Not Exist    ${SCAN_DIRECTORY}/eicar.com

    Restore Threat In SafeStore Database By ThreatId    ${threatId}

    ${dbContentAfterRestore} =    Get Contents of SafeStore Database
    Should Not Be Equal    ${dbContentBeforeRestore}    ${dbContentAfterRestore}
    File Should Exist    ${SCAN_DIRECTORY}/eicar.com

Correlation Id Stays The Same Until The File Is Quarantined And Changes For Redetection After Quarantine
    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag not set. Setting SafeStore to disabled.    timeout=60

    Check avscanner can detect eicar

    ${correlation_id1} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id1}
    ...  succeeded=0
    ...  origin=1
    ...  result=3
    ...  path=${SCAN_DIRECTORY}/eicar.com

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Check avscanner can detect eicar

    # We get a successful clean event
    Wait Until Base Has Core Clean Event
        ...  alert_id=${correlation_id1}
        ...  succeeded=1
        ...  origin=1
        ...  result=0
        ...  path=${SCAN_DIRECTORY}/eicar.com

    # Remove events to make sure we pick up the new ones
    Empty Directory  ${MCS_PATH}/event
    Check avscanner can detect eicar

    ${correlation_id2} =  Wait Until Base Has Detection Event
    ...  user_id=n/a
    ...  name=EICAR-AV-Test
    ...  threat_type=1
    ...  origin=1
    ...  remote=false
    ...  sha256=275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f
    ...  path=${SCAN_DIRECTORY}/eicar.com

    Wait Until Base Has Core Clean Event
    ...  alert_id=${correlation_id2}
    ...  succeeded=1
    ...  origin=1
    ...  result=0
    ...  path=${SCAN_DIRECTORY}/eicar.com

    Should Not Be Equal  ${correlation_id1}  ${correlation_id2}

Threat Is Re-detected By On-access After Being Cached If Removed From Allow-list
    ${ss_mark} =  Get SafeStore Log Mark
    ${av_mark} =  Get AV Log Mark
    ${oa_mark} =  Get on access log mark
    Send Policies to enable on-access  flags_policy/flags_safestore_onaccess_and_ml_enabled.json  ${oa_mark}
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60
    wait_for_log_contains_from_mark  ${av_mark}  On-access is enabled in the FLAGS policy, assuming on-access policy settings
    wait_for_log_contains_from_mark  ${av_mark}  SafeStore will quarantine ML detections
    Wait Until SafeStore running

    # Detect using on-access
    ${allow_listed_threat_file} =  Set Variable  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${allow_listed_threat_file}
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${allow_listed_threat_file} successfully
    File Should Not Exist  ${allow_listed_threat_file}

    # Allow-list the file and wait for it to be restored
    ${ss_mark} =  Get SafeStore Log Mark
    Send CORC Policy To Base   corc_policy.xml
    Wait For Log Contains From Mark  ${ss_mark}  SafeStore Database Rescan request received
    Wait For Log Contains From Mark  ${ss_mark}  Rescan found quarantined file no longer a threat: ${allow_listed_threat_file}
    Wait For Log Contains From Mark  ${ss_mark}  RestoreReportingClient reports successful restoration of ${allow_listed_threat_file}
    File Should Exist  ${allow_listed_threat_file}

    # Perform an on-access detection on the file to make sure it is cached as safe
    ${oa_mark} =  get_on_access_log_mark
    Open And Close File  ${allow_listed_threat_file}
    Wait For Log Contains From Mark  ${oa_mark}  caching ${allow_listed_threat_file}

    # Clear the allow list and wait for it to be received
    ${ss_mark} =  Get SafeStore Log Mark
    ${td_mark} =  mark_log_size  ${THREAT_DETECTOR_LOG_PATH}
    Send CORC Policy To Base   corc_policy_empty_allowlist.xml
    Wait For Log Contains From Mark  ${ss_mark}  SafeStore Database Rescan request received

    # Re-detect the file with on access
    ${ss_mark} =  Get SafeStore Log Mark
    Open And Close File  ${allow_listed_threat_file}
    Wait For Log Contains From Mark  ${ss_mark}  Quarantined ${allow_listed_threat_file} successfully
    File Should Not Exist  ${allow_listed_threat_file}


*** Keywords ***
SafeStore Test Setup
    Require Plugin Installed and Running  DEBUG
    LogUtils.save_log_marks_at_start_of_test
    Start SafeStore
    Wait Until SafeStore Started Successfully

    Create Directory  ${NORMAL_DIRECTORY}

    # Start from known place with a CORC policy with an empty allow list
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Empty Directory  ${MCS_PATH}/event

    register on fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register on fail  dump log  ${WATCHDOG_LOG}
    register on fail  dump log  ${SAFESTORE_LOG_PATH}
    register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/wdctl.log
    register on fail  dump log  ${SOPHOS_INSTALL}/plugins/av/log/av.log
    register on fail  dump log   ${SUSI_DEBUG_LOG_PATH}
    register on fail  dump log   ${ON_ACCESS_LOG_PATH}
    register on fail  dump threads  ${SOPHOS_THREAT_DETECTOR_BINARY}
    register on fail  dump threads  ${PLUGIN_BINARY}
    register on fail  analyse Journalctl   print_always=True

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

Start On Access And SafeStore
    ${av_mark} =  Get AV Log Mark
    ${oa_mark} =  Get on access log mark

    Send Policies to enable on-access  flags_policy/flags_enabled.json  ${oa_mark}

    wait_for_log_contains_from_mark  ${av_mark}  SafeStore flag set. Setting SafeStore to enabled.    timeout=60
    wait_for_log_contains_from_mark  ${av_mark}  On-access is enabled in the FLAGS policy, assuming on-access policy settings
