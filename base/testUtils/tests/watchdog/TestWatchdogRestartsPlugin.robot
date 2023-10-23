*** Settings ***
Documentation   Test wdctl can ask watchdog to restart a process

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Force Tags    TAP_PARALLEL6    WATCHDOG    WDCTL

Test Setup  Require Fresh Install

*** Test Cases ***
Test wdctl can ask watchdog to restart a process
    [Tags]    SMOKE
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds  Check Management Agent Running And Ready
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   managementagent
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds  check managementagent not running
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   managementagent
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds  Check Management Agent Running And Ready

Test wdctl can add a new plugin to running watchdog
    Remove File  /tmp/TestWdctlCanAddANewPluginToRunningWatchdog
    Remove File  ${SOPHOS_INSTALL}/base/pluginRegistry/testplugin.json
    Setup Test Plugin Config  echo "Plugin started at $(date)" >>/tmp/TestWdctlCanAddANewPluginToRunningWatchdog   ${TEMPDIR}  testplugin
    ## call wdctl to copy configuration
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   copyPluginRegistration    ${TEMPDIR}/testplugin.json

    ## call wdctl to start plugin
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start    testplugin

    ## Verify plugin has been run
    Wait For Marker To Be Created  /tmp/TestWdctlCanAddANewPluginToRunningWatchdog  30

Test Watchdog Config is Updated When Watchdog is Asked to Restart a Process
    [Tags]    SMOKE
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds  Check Management Agent Running And Ready

    Wait Until Keyword Succeeds
    ...    30 secs
    ...    2 secs
    ...    Log File    ${WD_ACTUAL_USER_GROUP_IDS}
    ${watchdogConfOriginal}    Get Modified Time    ${WD_ACTUAL_USER_GROUP_IDS}

    Verify Watchdog Config

    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   managementagent
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds  check managementagent not running
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   managementagent
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds  Check Management Agent Running And Ready

    Wait Until Keyword Succeeds
    ...    10 secs
    ...    0.5 secs
    ...    Verify Watchdog Conf Has Changed    ${watchdogConfOriginal}
    Verify Watchdog Config
