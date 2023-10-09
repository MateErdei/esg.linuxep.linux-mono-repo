*** Settings ***
Documentation    Test the control of the logging settings in SSPL

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  WatchdogResources.robot
Resource  ../installer/InstallerResources.robot
Resource  LogControlResources.robot

Test Setup  Require Fresh Install
Test Teardown  Wdctl Test Teardown
*** Variables ***
${watchdog_log}     ${SOPHOS_INSTALL}/logs/base/watchdog.log

*** Test Cases ***
Logger Conf should Control Log Level of Management Agent
    [Tags]  MANAGEMENT_AGENT
    Set Log Level For Component And Reset and Return Previous Log  sophos_managementagent   DEBUG
    Sleep  3 secs
    Override LogConf File as Global Level  ERROR
    ${debugLevelLogs} =  Set Log Level For Component And Reset and Return Previous Log  sophos_managementagent   WARN
    Sleep  3 secs
    ${WARNLevelLogs} =  Get Log Content For Component And Clear It  sophos_managementagent

    Should Not Contain   ${WARNLevelLogs}  DEBUG
    Should Contain   ${debugLevelLogs}  DEBUG

Logger should rotate after a log file reachs 1Mb
    [Teardown]  CleanUp Mock Logs
    Run Process   systemctl  stop  sophos-spl
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Not Running

    Remove File  ${watchdog_log}

    generate_file  ${watchdog_log}  ${1024}

    Run Process   systemctl  start  sophos-spl
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Log Contains  ProcessMonitoringImpl
    File should exist   ${SOPHOS_INSTALL}/logs/base/watchdog.log.1

Logger should not rotate more than 10 times
    [Teardown]  CleanUp Mock Logs
    Run Process   systemctl  stop  sophos-spl
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Not Running

    Remove File  ${watchdog_log}

    generate_file  ${watchdog_log}  ${1024}
    Create file  ${watchdog_log}.1
    Create file  ${watchdog_log}.2
    Create file  ${watchdog_log}.3
    Create file  ${watchdog_log}.4
    Create file  ${watchdog_log}.5
    Create file  ${watchdog_log}.6
    Create file  ${watchdog_log}.7
    Create file  ${watchdog_log}.8
    Create file  ${watchdog_log}.9
    Create file  ${watchdog_log}.10

    Run Process   systemctl  start  sophos-spl
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Log Contains  ProcessMonitoringImpl
    File should not exist   ${SOPHOS_INSTALL}/logs/base/watchdog.log.11
Logger Conf should control Log Level of Plugins and their internal Components
    [Tags]  UPDATE_SCHEDULER
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log  updatescheduler   WARN  pluginapi=DEBUG

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check UpdateScheduler issues a plugin api Log

    Check UpdateScheduler report its log level   WARN

    Override LogConf File as Global Level  ERROR
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log  updatescheduler   DEBUG  pluginapi=WARN

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check UpdateScheduler report its log level  DEBUG

    Run Keyword and Expect Error  *does not contain*   Check UpdateScheduler issues a plugin api Log

Logger Conf should Control Log Level of MCS Router via Component Specific Variable
    [Tags]  MCS
    Set Log Level For Component And Reset and Return Previous Log  mcs_router   DEBUG
    Sleep  3 secs
    Override LogConf File as Global Level  ERROR
    ${debugLevelLogs} =  Set Log Level For Component And Reset and Return Previous Log  mcs_router   WARN
    Sleep  3 secs
    ${WARNLevelLogs} =  Get Log Content For Component And Clear It  mcs_router

    Should Not Contain   ${WARNLevelLogs}  DEBUG
    Should Contain   ${debugLevelLogs}  DEBUG

Logger Conf should note crash MCSRouter if it has incorrect permissions
    [Tags]  MCS
    Set Log Level For Component And Reset and Return Previous Log  mcs_router   DEBUG
    Sleep  3 secs
    Override LogConf File as Global Level  ERROR
    ${debugLevelLogs} =  Get Log Content For Component And Clear It  mcs_router
    Should Contain   ${debugLevelLogs}  DEBUG
    Run Process  chown  root:root  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Run Process  chmod  0660  ${SOPHOS_INSTALL}/base/etc/logger.conf
    #Restart Plugin And Return Its Log File  mcsrouter  mcs_router
    Run Keyword and Ignore Error   Wdctl stop plugin  mcsrouter
    Wdctl Start Plugin  mcsrouter
    Sleep  3 secs
    ${infoLevelLogs} =  Get Log Content For Component And Clear It  mcs_router

    Should Not Contain   ${infoLevelLogs}  DEBUG
    Should Contain       ${infoLevelLogs}  Log config file exists but is either empty or cannot be read.

Logger Conf should Control Log Level of MCS Router via Global Variable
    [Tags]  MCS
    Override LogConf File as Global Level  DEBUG
    Restart Plugin And Return Its Log File  mcsrouter  mcs_router
    Sleep  3 secs
    ${debugLevelLogs} =  Get Log Content For Component And Clear It  mcs_router

    Should Contain   ${debugLevelLogs}  DEBUG

Logger Conf should not Control Log Level of MCS Router if Variables are invalid
    [Tags]  MCS
    Override LogConf File as Global Level  notarealloglevel
    Set Log Level For Component And Reset and Return Previous Log  mcs_router   alsonotarealloglevel
    Sleep  3 secs
    ${invalidLevelLogs} =  Get Log Content For Component And Clear It  mcs_router

    Should Not Contain   ${invalidLevelLogs}  DEBUG
    Should Contain   ${invalidLevelLogs}  INFO

Logger Conf sets Log Level of MCS Router to Warning via Global Variable
    [Tags]  MCS
    Override LogConf File as Global Level  WARN
    Restart Plugin And Return Its Log File  mcsrouter  mcs_router
    Sleep  3 secs
    ${warnLevelLogs} =  Get Log Content For Component And Clear It  mcs_router

    Should Contain   ${warnLevelLogs}  WARNING
    Should Not Contain   ${warnLevelLogs}  INFO

Logger Conf should not Control Log Level of MCS Router if no verbosity fields
    [Tags]  MCS
    Override LogConf File as Global Level  notarealloglevel  FAKEKEY
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log  mcs_router   alsonotarealloglevel  FAKEVALUE
    Sleep  3 secs
    ${invalidLevelLogs} =  Get Log Content For Component And Clear It  mcs_router

    Should Not Contain   ${invalidLevelLogs}  DEBUG
    Should Contain   ${invalidLevelLogs}  INFO


*** Keywords ***
Check UpdateScheduler issues a plugin api Log
    ${content} =  Get Log Content For Component   updatescheduler
    Should Contain  ${content}  pluginapi

CleanUp Mock Logs
    Remove File  ${watchdog_log}
    Wdctl Test Teardown

Check UpdateScheduler report its log level
    [Arguments]  ${logLevelName}
    ${content} =  Get Log Content For Component   updatescheduler
    Should Contain  ${content}  configured for level: ${logLevelName}

