*** Settings ***
Library    OperatingSystem

Library    ${LIBS_DIRECTORY}/FaultInjectionTools.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource    AVResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot

*** Variables ***
${SAFESTORE_BIN}                    ${AV_PLUGIN_PATH}/sbin/safestore

${SAFESTORE_LOG_PATH}               ${AV_PLUGIN_PATH}/log/safestore.log
${SAFESTORE_PID_FILE}               ${AV_PLUGIN_PATH}/var/safestore.pid
${SAFESTORE_SOCKET_PATH}            ${AV_PLUGIN_PATH}/var/safestore_socket

${SAFESTORE_DB_DIR}                 ${AV_PLUGIN_PATH}/var/safestore_db
${SAFESTORE_DB_PATH}                ${SAFESTORE_DB_DIR}/safestore.db
${SAFESTORE_DB_PASSWORD_PATH}       ${SAFESTORE_DB_DIR}/safestore.pw

${THREAT_DATABASE_PATH}             ${AV_PLUGIN_PATH}/var/persist-threatDatabase

*** Keywords ***
Stop SafeStore
    ${result} =    RunProcess    ${SOPHOS_INSTALL}/bin/wdctl    stop    safestore
    Should Be Equal As Integers    ${result.rc}    ${0}

Start SafeStore
    ${result} =    RunProcess    ${SOPHOS_INSTALL}/bin/wdctl    start    safestore
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
    ...    Check SafeStore Log Contains    Quarantine Manager initialised OK
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
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