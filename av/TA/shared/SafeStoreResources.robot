*** Settings ***
Resource    AVResources.robot
Resource    BaseResources.robot
Resource    GlobalSetup.robot
Resource    ${COMMON_TEST_ROBOT}/SafeStoreResources.robot

*** Variables ***
${SAFESTORE_BIN}                      ${COMPONENT_ROOT_PATH}/sbin/safestore

${SAFESTORE_LOG_PATH}                 ${AV_PLUGIN_PATH}/log/safestore.log
${SAFESTORE_PID_FILE}                 ${COMPONENT_VAR_DIR}/safestore.pid
${SAFESTORE_SOCKET_PATH}              ${COMPONENT_VAR_DIR}/safestore_socket

${SAFESTORE_DB_DIR}                   ${COMPONENT_VAR_DIR}/safestore_db
${SAFESTORE_DB_PATH}                  ${SAFESTORE_DB_DIR}/safestore.db
${SAFESTORE_DB_PASSWORD_PATH}         ${SAFESTORE_DB_DIR}/safestore.pw

${SAFESTORE_TELEMETRY_BACKUP_JSON}    ${SSPL_BASE}/telemetry/cache/safestore-telemetry.json

${THREAT_DATABASE_PATH}               ${COMPONENT_VAR_DIR}/persist-threatDatabase
${DISABLE_SAFESTORE_FILEPATH}         ${AV_VAR_DIR}/disable_safestore

*** Keywords ***
Start SafeStore Manually
    ${handle} =  Start Process  ${SAFESTORE_BIN}  stdout=DEVNULL  stderr=DEVNULL
    Set Test Variable  ${SAFESTORE_HANDLE}  ${handle}


Stop SafeStore Manually
    ${result} =  Terminate Process  ${SAFESTORE_HANDLE}
    Set Suite Variable  ${SAFESTORE_HANDLE}  ${None}


Wait Until SafeStore running
    [Arguments]  ${timeout}=${60}
    Wait For Pid  ${SAFESTORE_BIN}  ${timeout}

    Wait_For_Log_contains_after_last_restart  ${SAFESTORE_LOG_PATH}  safestore <> SafeStore started  timeout=10

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Verify SafeStore Database Exists


Wait Until SafeStore Started Successfully
    Wait Until SafeStore running
    ## password creation only done on first run - can't cover complete log turn-over so check for existance of file:
    Wait Until Created    ${SAFESTORE_DB_PASSWORD_PATH}

    ## Lines logged for every start
    Wait_For_Log_contains_after_last_restart  ${SAFESTORE_LOG_PATH}  Quarantine Manager initialised OK  timeout=15
    Wait_For_Log_contains_after_last_restart  ${SAFESTORE_LOG_PATH}  Successfully initialised SafeStore database  timeout=5
    Wait_For_Log_contains_after_last_restart  ${SAFESTORE_LOG_PATH}  SafeStore started  timeout=5

SafeStore Log Contains
    [Arguments]  ${input}
    File Log Contains     ${SAFESTORE_LOG_PATH}   ${input}

Wait Until SafeStore Log Contains
    [Arguments]  ${input}  ${timeout}=15  ${interval}=0
    ${interval} =   Set Variable If
    ...   ${interval} > 0   ${interval}
    ...   ${timeout} >= 120   10
    ...   ${timeout} >= 60   5
    ...   ${timeout} >= 15   3
    ...   1
    Wait Until File Log Contains  SafeStore Log Contains   ${input}   timeout=${timeout}  interval=${interval}


Check SafeStore Telemetry
    [Arguments]    ${telemetryKey}    ${telemetryValue}
    Run Telemetry Executable With HTTPS Protocol

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json

    Log    ${telemetryJson}
    Dictionary Should Contain Key    ${telemetryJson}    safestore
    Dictionary Should Contain Item   ${telemetryJson["safestore"]}   ${telemetryKey}   ${telemetryValue}

Cleanup Safestore Database
    Stop Safestore
    Remove Directory    ${SAFESTORE_DB_DIR}    recursive=True
    Start Safestore

Corrupt SafeStore Database
    Stop SafeStore
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/persist-safeStoreDbErrorThreshold    1
    Remove Files    ${SAFESTORE_DB_PATH}    ${SAFESTORE_DB_PASSWORD_PATH}
    Copy Files    ${RESOURCES_PATH}/safestore_db_corrupt/*    ${SAFESTORE_DB_DIR}
    Start SafeStore

Corrupt SafeStore Database With Lock
    Stop SafeStore
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/persist-safeStoreDbErrorThreshold    1
    Create Directory   ${SAFESTORE_DB_DIR}/safestore.db.lock
    Remove Files    ${SAFESTORE_DB_PATH}    ${SAFESTORE_DB_PASSWORD_PATH}
    Copy Files    ${RESOURCES_PATH}/safestore_db_corrupt/*    ${SAFESTORE_DB_DIR}
    Start SafeStore

Verify SafeStore Database Exists
    Directory Should Exist    ${SAFESTORE_DB_DIR}
    File Should Exist    ${SAFESTORE_DB_PATH}
    File Should Exist    ${SAFESTORE_DB_PASSWORD_PATH}

Verify SafeStore Database Backups Exist in Path
    [Arguments]    ${pathToCheck}
    Directory Should Exist  ${pathToCheck}

    ${safeStoreDatabaseBackupDirs} =    List Directories In Directory    ${pathToCheck}
    Should Not Be Empty    ${safeStoreDatabaseBackupDirs}

    FOR   ${dir}  IN  @{safeStoreDatabaseBackupDirs}
        Directory Should Not Be Empty    ${SAFESTORE_BACKUP_DIR}/${dir}
        File Should Exist  ${SAFESTORE_BACKUP_DIR}/${dir}/safestore.db
        File Should Exist  ${SAFESTORE_BACKUP_DIR}/${dir}/safestore.pw
    END

Remove All But One SafeStore Backup
    ${safeStoreDatabaseBackupDirs} =    List Directories In Directory    ${AV_RESTORED_VAR_DIRECTORY}
    ${numberOfSSDatabases}=    Get length    ${safeStoreDatabaseBackupDirs}
    IF  ${1} < ${numberOfSSDatabases}
        FOR   ${dir}  IN  @{safeStoreDatabaseBackupDirs}[1:]
            Remove Directory    ${AV_RESTORED_VAR_DIRECTORY}/${dir}  recursive=True
            Remove Values From List    ${safeStoreDatabaseBackupDirs}    ${dir}
        END
    END

Disable Safestore
    [Arguments]    ${file_path}=${DISABLE_SAFESTORE_FILEPATH}
    Create File    ${file_path}

Enable Safestore
    [Arguments]    ${file_path}=${DISABLE_SAFESTORE_FILEPATH}
    Remove File    ${file_path}