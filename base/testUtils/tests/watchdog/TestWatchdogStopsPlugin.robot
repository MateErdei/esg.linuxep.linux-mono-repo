*** Settings ***
Documentation   Test wdctl can ask watchdog to stop a process

Library    Process
Library    DateTime
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

Test wdctl will block on removing a plugin
    [Tags]    WATCHDOG  WDCTL   SMOKE

    ${result} =    Run Process   systemctl  stop  sophos-spl

    setup_test_plugin_config_with_given_executable  SystemProductTestOutput/ignoreSignals
    ${result} =    Run Process   systemctl  start  sophos-spl
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains  Starting /opt/sophos-spl/testPlugin  ${SOPHOS_INSTALL}/logs/base/watchdog.log  watchdog log
    ${date} =	Get Current Date
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   removePluginRegistration   fakePlugin
    ${date1} =	Get Current Date
    ${TimeDiff} =	Subtract Date From Date  ${date1}  ${date}
    IF    ${TimeDiff} < 4.5
        Fail
    END
    check_watchdog_log_contains  Killing process
    check_watchdog_log_contains  Output

Test watchdog logs plugin output during system shutdown
    [Tags]    WATCHDOG

    setup_test_plugin_config_with_given_executable  SystemProductTestOutput/ignoreSignals
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   fakePlugin
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains  Starting /opt/sophos-spl/testPlugin  ${SOPHOS_INSTALL}/logs/base/watchdog.log  watchdog log
    mark_watchdog_log
    ${result} =    Run Process   systemctl  stop  sophos-spl
    check_marked_watchdog_log_contains  Killing process
    check_marked_watchdog_log_contains  Output
*** Keywords ***
