*** Settings ***
Documentation   Test wdctl can ask watchdog to stop a process

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py

Resource  ../installer/InstallerResources.robot

Resource  WatchdogResources.robot

Test Setup  Require Fresh Install

*** Test Cases ***
Test wdctl can ask watchdog to stop a process
    [Tags]    WATCHDOG  WDCTL   SMOKE
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Management Agent Running And Ready
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   managementagent
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   check managementagent not running

*** Keywords ***
