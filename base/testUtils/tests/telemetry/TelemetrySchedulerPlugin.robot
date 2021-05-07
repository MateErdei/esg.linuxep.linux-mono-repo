*** Settings ***
Documentation  Test the Telemetry Scheduler's installation, uninstallation and watchdog interaction

Resource  TelemetryResources.robot
Resource  ../GeneralTeardownResource.robot

Suite Teardown   Cleanup Telemetry Scheduler Tests

Test Setup       Telemetry Scheduler Plugin Test Setup
Test Teardown    Telemetry Scheduler Plugin Test Teardown

Default Tags  TELEMETRY SCHEDULER

*** Keywords ***
### Suite Cleanup
Cleanup Telemetry Scheduler Tests
    Uninstall SSPL


Telemetry Scheduler Plugin Test Setup
    Require Fresh Install


Telemetry Scheduler Plugin Test Teardown
    General Test Teardown
    Terminate All Processes  kill=True

*** Test Cases ***
Check Telemetry Scheduler Plugin Is Started by Watchdog
    [Tags]  SMOKE  TELEMETRY SCHEDULER  TAP_TESTS
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Telemetry Scheduler Is Running


Check Telemetry Scheduler Plugin Is Stopped by Watchdog
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Telemetry Scheduler Is Running
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   tscheduler
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Telemetry Scheduler Plugin Not Running


Check Telemetry Scheduler Plugin Is Stopped When SSPL Is Stopped
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Telemetry Scheduler Is Running
    Stop Watchdog
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Telemetry Scheduler Plugin Not Running


Check Telemetry Scheduler Plugin Is Removed On Uninstallation
    Ensure Uninstalled
    Run Full Installer
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Telemetry Scheduler Is Running
    Run Process    ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    Verify Group Removed
    Verify User Removed
    Should Not Exist   ${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Telemetry Scheduler Plugin Not Running
