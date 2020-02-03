*** Settings ***
Documentation    Test watchdog restarts plugin after exit

Library    OperatingSystem
Library    ${libs_directory}/FullInstallerUtils.py
Library    ${libs_directory}/Watchdog.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Test Setup  Require Fresh Install
Test Teardown  Run Keywords  Kill Manual Watchdog  AND
...                          General Test Teardown

*** Test Cases ***
Test watchdog restarts child after normal exit
    [Tags]    WATCHDOG  SMOKE
    Remove File  /tmp/TestWatchdogRestartsChildAfterNormalExit
    Setup Test Plugin Config  echo "Plugin startedat $(date)" >>/tmp/TestWatchdogRestartsChildAfterNormalExit
    Manually Start Watchdog
    Wait For Plugin To Start  /tmp/TestWatchdogRestartsChildAfterNormalExit  30
    ## Then waits 10 seconds and restarts
    Wait For Plugin To Start Again  /tmp/TestWatchdogRestartsChildAfterNormalExit  30

Test watchdog restarts child after kill
    [Tags]    WATCHDOG
    Remove File  /tmp/TestWatchdogRestartsChildAfterKill
    Setup Test Plugin Config  echo "$$" >>/tmp/TestWatchdogRestartsChildAfterKill ; sleep 60
    Manually Start Watchdog
    Wait For Plugin To Start  /tmp/TestWatchdogRestartsChildAfterKill  30
    ## Kill plugin
    Kill Plugin  /tmp/TestWatchdogRestartsChildAfterKill
    ## Then waits 10 seconds and restarts
    Wait For Plugin To Start Again  /tmp/TestWatchdogRestartsChildAfterKill  30


*** Keywords ***