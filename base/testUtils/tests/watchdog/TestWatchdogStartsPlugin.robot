*** Settings ***
Documentation    Test Watchdog starts a subprocess

Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Test Setup  Require Fresh Install
Test Teardown  Run Keywords  Kill Manual Watchdog  AND
...                          General Test Teardown

*** Test Cases ***
Test Watchdog Starts Plugin
    [Tags]    WATCHDOG  SMOKE  TAP_TESTS
    Remove File  /tmp/TestWatchDogStartsPluginTestFile
    Setup Test Plugin Config  echo "Plugin started" > /tmp/TestWatchDogStartsPluginTestFile
    Manually Start Watchdog
    Wait For Marker To Be Created  /tmp/TestWatchDogStartsPluginTestFile  30


