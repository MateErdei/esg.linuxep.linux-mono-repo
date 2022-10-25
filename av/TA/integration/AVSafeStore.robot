*** Settings ***
Documentation    Product tests of SafeStore
Force Tags       INTEGRATION  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot

Library         ../Libs/CoreDumps.py
Library         ../Libs/OnFail.py
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


*** Test Cases ***

SafeStore Database is Initialised
    Wait Until Safestore Log Contains    Successfully saved SafeStore database password to file
    Wait Until SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    File Should Exist    ${SAFESTORE_DB_PASSWORD_PATH}

SafeStore Can Reinitialise Database Containing Threats
    Mark AV Log
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait Until AV Plugin Log Contains With Offset    SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until Safestore Log Contains    Successfully saved SafeStore database password to file
    Wait Until SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

    ${ssPassword1} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    Check avscanner can detect eicar
    Wait Until Safestore Log Contains  Received Threat:

    ${filesInSafeStoreDb1} =  List Files In Directory  ${SAFESTORE_DB_DIR}
    Log  ${filesInSafeStoreDb1}

    Stop SafeStore
    Check Safestore Not Running
    Mark SafeStore Log

    Start SafeStore
    Wait Until SafeStore Log Contains With Offset    Quarantine Manager initialised OK
    Wait Until SafeStore Log Contains With Offset    Successfully initialised SafeStore database

    Directory Should Not Be Empty    ${SAFESTORE_DB_DIR}
    ${ssPassword2} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    Should Be Equal As Strings    ${ssPassword1}    ${ssPassword2}

    ${filesInSafeStoreDb2} =  List Files In Directory  ${SAFESTORE_DB_DIR}
    Log  ${filesInSafeStoreDb2}

    # Removing tmp file: https://www.sqlite.org/tempfiles.html
    Remove Values From List    ${filesInSafeStoreDb1}    safestore.db-journal
    Should Be Equal    ${filesInSafeStoreDb1}    ${filesInSafeStoreDb2}

SafeStore Recovers From Corrupt Database
    Mark AV Log
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait Until AV Plugin Log Contains With Offset    SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until Safestore Log Contains    Successfully saved SafeStore database password to file
    Wait Until SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

    Mark SafeStore Log
    Corrupt SafeStore Database

    Wait Until SafeStore Log Contains With Offset    Successfully removed corrupt SafeStore database    200
    Wait Until SafeStore Log Contains With Offset    Successfully initialised SafeStore database

    Mark SafeStore Log
    Check avscanner can detect eicar
    Wait Until SafeStore Log Contains With Offset  Received Threat:

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise

SafeStore Quarantines When It Recieves A File To Quarantine
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Mark AV Log

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait Until AV Plugin Log Contains With Offset    SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Check avscanner can detect eicar

    Wait Until SafeStore Log Contains  Received Threat:
    Wait Until AV Plugin Log Contains With Offset  Quarantine succeeded
    File Should Not Exist   ${SCAN_DIRECTORY}/eicar.com

SafeStore does not quarantine on a Corrupt Database
    Mark AV Log
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait Until AV Plugin Log Contains With Offset    SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Wait Until Safestore Log Contains    Successfully saved SafeStore database password to file
    Wait Until SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

    Mark SafeStore Log
    Corrupt SafeStore Database
    Check avscanner can detect eicar

    Wait Until SafeStore Log Contains  Received Threat:
    Wait Until SafeStore Log Contains  Cannot quarantine file, SafeStore is in
    Wait Until SafeStore Log Contains With Offset    Successfully removed corrupt SafeStore database    200
    Wait Until SafeStore Log Contains With Offset    Successfully initialised SafeStore database

    Mark SafeStore Log
    Check avscanner can detect eicar
    Wait Until SafeStore Log Contains With Offset  Received Threat:

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise

With SafeStore Enabled But Not Running We Can Send Threats To AV
    register cleanup    Exclude Watchdog Log Unable To Open File Error

    Stop SafeStore
    Mark AV Log

    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait Until AV Plugin Log Contains With Offset    SafeStore flag set. Setting SafeStore to enabled.    timeout=60

    Check avscanner can detect eicar

    Wait Until AV Plugin Log Contains With Offset  <notification description="Found 'EICAR-AV-Test'
    Wait Until AV Plugin Log Contains With Offset  Failed to write to SafeStore socket.
    Check SafeStore Not Running


*** Keywords ***
SafeStore Test Setup
    Require Plugin Installed and Running  DEBUG
    Run Keyword and Ignore Error   Run Shell Process   ${SOPHOS_INSTALL}/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter
    Start SafeStore

    Mark AV Log
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

Corrupt SafeStore Database
    Stop SafeStore
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/persist-safeStoreDbErrorThreshold    1

    Remove Files    ${SAFESTORE_DB_PATH}    ${SAFESTORE_DB_PASSWORD_PATH}
    Copy Files    ${RESOURCES_PATH}/safestore_db_corrupt/*    ${SAFESTORE_DB_DIR}
    Start SafeStore