*** Settings ***
Library    OperatingSystem

Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource    AVResources.robot

*** Variables ***
${SAFESTORE_BIN}                    ${AV_PLUGIN_PATH}/sbin/safestore

${SAFESTORE_LOG_PATH}               ${AV_PLUGIN_PATH}/log/safestore.log
${SAFESTORE_PID_FILE}               ${AV_PLUGIN_PATH}/var/safestore.pid
${SAFESTORE_SOCKET_PATH}            ${AV_PLUGIN_PATH}/var/safestore_socket

${AV_RESTORED_VAR_DIRECTORY}        ${AV_PLUGIN_PATH}/var/downgrade-backup
${SAFESTORE_DB_DIR}                 ${AV_PLUGIN_PATH}/var/safestore_db
${SAFESTORE_DB_PATH}                ${SAFESTORE_DB_DIR}/safestore.db
${SAFESTORE_DB_PASSWORD_PATH}       ${SAFESTORE_DB_DIR}/safestore.pw

${THREAT_DATABASE_PATH}             ${AV_PLUGIN_PATH}/var/persist-threatDatabase

*** Keywords ***
Stop SafeStore
    ${result} =    RunProcess    ${SOPHOS_INSTALL}/bin/wdctlstopsafestore
    Should Be Equal As Integers    ${result.rc}    ${0}

Start SafeStore
    ${result} =    RunProcess    ${SOPHOS_INSTALL}/bin/wdctlstartsafestore
    Should Be Equal As Integers    ${result.rc}    ${0}

Check SafeStore Running
    ${result} =    Get SafeStore PID
    Should Be Equal As Integers    ${result}    ${0}

Check SafeStore Not Running
    ${result} =    Get SafeStore PID
    Should Be Equal As Integers    ${result}    ${-1}

Get SafeStore PID
    ${pid} =     Run Process    pgrep    -f    ${SAFESTORE_BIN}
    [Return]    ${pid.stdout}

Check SafeStore Permissions And Owner
    ${safeStorePid} =    Get SafeStore PID
    ${watchdogPid} =    Run Process    pgrep    sophos_watchdog

    ${user} =    Run Process    ps    -o    user    -p    ${safeStorePid.stdout}
    ${group} =    Run Process    ps    -o    group    -p    ${safeStorePid.stdout}
    ${parentPid} =    Run Process    ps    -o    ppid    -p    ${safeStorePid.stdout}

    Should Contain    ${user.stdout}    root
    Should Contain    ${group.stdout}    root
    Should Be Equal As Integers    ${parentPid.stdout}    ${watchdogPid.stdout}

Check SafeStore Installed Correctly
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    AV Plugin Log Contains    SafeStore flag set. Setting SafeStore to enabled

    File Should Exist    ${SAFESTORE_BIN}
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    Check SafeStore Running
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    SafeStore Log Contains    SafeStore started
    Check SafeStore Database Exists
    Check SafeStore Permissions And Owner

Check SafeStore Upgraded Correctly with Persisted Database
    [Arguments]    ${oldDatabaseDirectory}    ${oldDatabase}    ${oldPassword}

    Check Marked AV Log Contains    Successfully restored old SafeStore database

    File Should Exist    ${SAFESTORE_BIN}
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    Check SafeStore Running
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    Check Marked SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    Check Marked SafeStore Log Contains    SafeStore started
    Check SafeStore Database Exists
    Check SafeStore Permissions And Owner
    Check SafeStore Database Has Not Changed    ${oldDatabaseDirectory}    ${oldDatabase}    ${oldPassword}

Check SafeStore Database Exists
    Directory Should Exist    ${SAFESTORE_DB_DIR}

    File Exists With Permissions    ${SAFESTORE_DB_PATH}    root    root    -rw-------
    File Exists With Permissions    ${SAFESTORE_DB_PASSWORD_PATH}    root    root    -rw-------

Check SafeStore Database Has Not Changed
    [Arguments]    ${oldDatabaseDirectory}    ${oldDatabase}    ${oldPassword}
    ${currentDatabaseDirectory} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${currentDatabase} =    Get File    ${SAFESTORE_DB_PATH}
    ${currentPassword} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    # Removing tmp file: https://www.sqlite.org/tempfiles.html
    Remove Values From List    ${oldDatabaseDirectory}    safestore.db-journal
    Should Be Equal    ${oldDatabaseDirectory}    ${currentDatabaseDirectory}

    Should Be Equal As Strings    ${oldDatabase}    ${currentDatabase}
    Should Be Equal As Strings    ${oldPassword}    ${currentPassword}


SafeStore Log Contains
    [Arguments]    ${textToFind}
    File Should Exist    ${SAFESTORE_LOG_PATH}
    ${fileContent} =    Get File    ${SAFESTORE_LOG_PATH}
    Should Contain    ${fileContent}    ${textToFind}

Remove All But One SafeStore Backup And Return Backup
    ${safeStoreDatabaseBackupDirs} =    List Directories In Directory    ${AV_RESTORED_VAR_DIRECTORY}
    ${numberOfSSDatabases}=    Get Length    ${safeStoreDatabaseBackupDirs}
    IF  ${1} < ${numberOfSSDatabases}
        FOR   ${dir}  IN  @{safeStoreDatabaseBackupDirs}[1:]
            Remove Directory    ${AV_RESTORED_VAR_DIRECTORY}/${dir}  recursive=True
            Remove Values From List    ${safeStoreDatabaseBackupDirs}    ${dir}
        END
    END

    [Return]    ${AV_RESTORED_VAR_DIRECTORY}/${safeStoreDatabaseBackupDirs[0]}