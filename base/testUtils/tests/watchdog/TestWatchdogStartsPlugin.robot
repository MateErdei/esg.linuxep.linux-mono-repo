*** Settings ***
Documentation    Test Watchdog starts a subprocess

Library    OperatingSystem
Library    ${libs_directory}/FullInstallerUtils.py
Library    ${libs_directory}/Watchdog.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Test Setup  Require Fresh Install
Test Teardown  Run Keywords  Kill Manual Watchdog  AND
...                          General Test Teardown

*** Test Cases ***
Test Watchdog Starts Plugin
    [Tags]    WATCHDOG  SMOKE
    Remove File  /tmp/TestWatchDogStartsPluginTestFile
    Setup Test Plugin Config  echo "Plugin started" > /tmp/TestWatchDogStartsPluginTestFile
    Manually Start Watchdog
    Wait For Marker To Be Created  /tmp/TestWatchDogStartsPluginTestFile  30


