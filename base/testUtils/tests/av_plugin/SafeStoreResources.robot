*** Settings ***
Library    OperatingSystem

Library    ${LIBS_DIRECTORY}/FaultInjectionTools.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/SafeStoreUtils.py

Resource    AVResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot

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
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl    stop    safestore
    Should Be Equal As Integers    ${result.rc}    ${0}

Start SafeStore
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl    start    safestore
    Should Be Equal As Integers    ${result.rc}    ${0}

Check SafeStore Running
    ${result} =    Run Process    pgrep    safestore
    Should Be Equal As Integers    ${result.rc}    ${0}

Check SafeStore Not Running
    ${result} =    Run Process    pgrep    safestore
    Should Be Equal As Integers    ${result.rc}    ${-1}

Get SafeStore PID
    ${pid} =     Run Process    pgrep    safestore
    [Return]    ${pid.stdout}

Check SafeStore Permissions And Owner
    ${safeStorePid} =    Get SafeStore PID

    ${user} =    Run Process    ps -o user -p ${safeStorePid}    shell=True
    ${group} =    Run Process    ps -o group -p ${safeStorePid}    shell=True
    Should Contain    ${user.stdout}    root
    Should Contain    ${group.stdout}    root

    ${safestoreparentpid} =    Run Process    ps -o ppid\= -p ${safeStorePid}    shell=True
    ${parentPid} =    Run Process    ps -e | grep ${safestoreparentpid.stdout}    shell=True
    Should Contain    ${parentPid.stdout}    sophos_watchdog

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
    ...    30 secs
    ...    2 secs
    ...    Check SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until Keyword Succeeds
    ...    30 secs
    ...    2 secs
    ...    Check SafeStore Log Contains    SafeStore started
    Check SafeStore Database Exists
    Check SafeStore Permissions And Owner

Check SafeStore Database Exists
    Directory Should Exist    ${SAFESTORE_DB_DIR}

    File Exists With Permissions    ${SAFESTORE_DB_PATH}    root    root    -rw-------
    File Exists With Permissions    ${SAFESTORE_DB_PASSWORD_PATH}    root    root    -rw-------

Toggle SafeStore Flag in MCS Policy
    [Arguments]    ${enabled}
    Copy File    ${SOPHOS_INSTALL}/base/mcs/policy/flags.json    /tmp/flags.json
    Modify Value In Json File    safestore.enabled    ${enabled}    /tmp/flags.json
    ${modifiedFlags} =    Get File    /tmp/flags.json

    Send Mock Flags Policy    ${modifiedFlags}

Check SafeStore Database Has Not Changed
    [Arguments]    ${oldDatabaseDirectory}    ${oldDatabaseContent}    ${oldPassword}
    ${currentDatabaseDirectory} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${currentPassword} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    # Removing tmp file: https://www.sqlite.org/tempfiles.html
    Remove Values From List    ${oldDatabaseDirectory}    safestore.db-journal
    Should Be Equal    ${oldDatabaseDirectory}    ${currentDatabaseDirectory}

    ${currentDatabaseContent} =    Get Contents of SafeStore Database

    Should Be Equal As Strings    ${oldDatabaseContent}    ${currentDatabaseContent}
    Should Be Equal As Strings    ${oldPassword}    ${currentPassword}

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
