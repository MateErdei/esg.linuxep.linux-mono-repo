*** Settings ***
Documentation  Test the Telemetry Scheduler's installation, uninstallation and watchdog interaction

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Suite Teardown   Cleanup Telemetry Save Restore Tests

Test Setup       Telemetry Save Restore Test Setup
Test Teardown    Telemetry Save Restore Test Teardown

Force Tags  TELEMETRY  WATCHDOG  TAP_PARALLEL5

*** Variables ***
${TELEMETRY_CACHE_DIR}  ${SOPHOS_INSTALL}/base/telemetry/cache

*** Test Cases ***
Check Save And Restore Correctly Persists Telemetry Through SSPL Restarts

    # MCS Router won't be running because it has not been registered, so it exits.
    Wait For Base Processes To Be Running Except MCS Router
    Kill Sophos Processes That Arent Watchdog
    Wait For Base Processes To Be Running Except MCS Router    ${30}

    Stop Watchdog
    Wait Until Keyword Succeeds  5 seconds  1 seconds   Check Telemetry Scheduler Plugin Not Running

    Check All Base Saved Telemetry Files Exists

    ${watchdogTelemetryContent}=  Get File   ${TELEMETRY_CACHE_DIR}/watchdogservice-telemetry.json
    Log  ${watchdogTelemetryContent}
    Check Watchdog Saved Json Strings Are Equal  ${watchdogTelemetryContent}  1

    Start Watchdog
    Wait For Base Processes To Be Running Except MCS Router    ${30}

    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check All Base Saved Telemetry Files Are Cleanup After Restore Exists

    Kill Sophos Processes That Arent Watchdog
    Wait For Base Processes To Be Running Except MCS Router    ${30}

    Stop Watchdog
    Wait Until Keyword Succeeds  5 seconds  1 seconds   Check Telemetry Scheduler Plugin Not Running
    ${watchdogTelemetryContent}=  Get File   ${TELEMETRY_CACHE_DIR}/watchdogservice-telemetry.json
    Log  ${watchdogTelemetryContent}
    Check Watchdog Saved Json Strings Are Equal  ${watchdogTelemetryContent}  2

*** Keywords ***
### Suite Cleanup
Cleanup Telemetry Save Restore Tests
    Uninstall SSPL

Telemetry Save Restore Test Setup
    Require Fresh Install


Telemetry Save Restore Test Teardown
    General Test Teardown
    Terminate All Processes  kill=True

Check All Base Saved Telemetry Files Exists
    File Should Exist  ${TELEMETRY_CACHE_DIR}/sophos_managementagent-telemetry.json
    File Should Exist  ${TELEMETRY_CACHE_DIR}/tscheduler-telemetry.json
    File Should Exist  ${TELEMETRY_CACHE_DIR}/watchdogservice-telemetry.json
    File Should Exist  ${TELEMETRY_CACHE_DIR}/updatescheduler-telemetry.json


Check All Base Saved Telemetry Files Are Cleanup After Restore Exists
    File Should Not Exist  ${TELEMETRY_CACHE_DIR}/sophos_managementagent-telemetry.json
    File Should Not Exist  ${TELEMETRY_CACHE_DIR}/tscheduler-telemetry.json
    File Should Not Exist  ${TELEMETRY_CACHE_DIR}/watchdogservice-telemetry.json
    File Should Not Exist  ${TELEMETRY_CACHE_DIR}/updatescheduler-telemetry.json
