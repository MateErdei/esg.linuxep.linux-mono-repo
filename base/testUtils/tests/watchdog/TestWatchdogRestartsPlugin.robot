*** Settings ***
Documentation   Test wdctl can ask watchdog to restart a process

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py

Resource  ../installer/InstallerResources.robot
Resource  WatchdogResources.robot

Test Setup  Require Fresh Install

*** Test Cases ***
Test wdctl can ask watchdog to restart a process
    [Tags]    WATCHDOG  WDCTL  SMOKE
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
    [Tags]    WATCHDOG  WDCTL
    Remove File  /tmp/TestWdctlCanAddANewPluginToRunningWatchdog
    Remove File  ${SOPHOS_INSTALL}/base/pluginRegistry/testplugin.json
    Setup Test Plugin Config  echo "Plugin started at $(date)" >>/tmp/TestWdctlCanAddANewPluginToRunningWatchdog   ${TEMPDIR}  testplugin
    ## call wdctl to copy configuration
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   copyPluginRegistration    ${TEMPDIR}/testplugin.json

    ## call wdctl to start plugin
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start    testplugin

    ## Verify plugin has been run
    Wait For Marker To Be Created  /tmp/TestWdctlCanAddANewPluginToRunningWatchdog  30

