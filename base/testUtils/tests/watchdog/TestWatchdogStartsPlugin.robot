*** Settings ***
Documentation    Test Watchdog starts a subprocess

Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/Watchdog.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot

Test Setup  Require Fresh Install
Test Teardown  Run Keywords  Kill Manual Watchdog  AND
...                          General Test Teardown

*** Test Cases ***
Test Watchdog Starts Plugin
    [Tags]    WATCHDOG  SMOKE  TAP_PARALLEL2
    Remove File  /tmp/TestWatchDogStartsPluginTestFile
    Setup Test Plugin Config  echo "Plugin started" > /tmp/TestWatchDogStartsPluginTestFile
    Manually Start Watchdog
    Wait For Marker To Be Created  /tmp/TestWatchDogStartsPluginTestFile  30


