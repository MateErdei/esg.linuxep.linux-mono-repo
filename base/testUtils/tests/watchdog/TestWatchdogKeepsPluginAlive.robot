*** Settings ***
Documentation    Test watchdog restarts plugin after exit

Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/Watchdog.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot

Force Tags    TAP_PARALLEL2    WATCHDOG

Test Setup  Require Fresh Install
Test Teardown  Run Keywords  Kill Manual Watchdog  AND
...                          General Test Teardown

*** Test Cases ***
Test watchdog restarts child after normal exit
    [Tags]    SMOKE
    Remove File  /tmp/TestWatchdogRestartsChildAfterNormalExit
    Setup Test Plugin Config  echo "Plugin startedat $(date)" >>/tmp/TestWatchdogRestartsChildAfterNormalExit
    Manually Start Watchdog
    Wait For Plugin To Start  /tmp/TestWatchdogRestartsChildAfterNormalExit  30
    ## Then waits 10 seconds and restarts
    Wait For Plugin To Start Again  /tmp/TestWatchdogRestartsChildAfterNormalExit  30

Test watchdog restarts child after kill
    Remove File  /tmp/TestWatchdogRestartsChildAfterKill
    Setup Test Plugin Config  echo "$$" >>/tmp/TestWatchdogRestartsChildAfterKill ; sleep 60
    Manually Start Watchdog
    Wait For Plugin To Start  /tmp/TestWatchdogRestartsChildAfterKill  30
    ## Kill plugin
    Kill Plugin  /tmp/TestWatchdogRestartsChildAfterKill
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Watchdog Log Contains  /opt/sophos-spl/testPlugin.sh died with signal 15
    ## Then waits 10 seconds and restarts
    Wait For Plugin To Start Again  /tmp/TestWatchdogRestartsChildAfterKill  30

Test watchdog restarts child quickly after restart 77 exit
    Remove File  /tmp/TestWatchdogRestartsChildAfterRestartExit
    Setup Test Plugin Config  echo "Plugin startedat $(date)" >>/tmp/TestWatchdogRestartsChildAfterRestartExit ; sleep 1 ; exit 77
    Manually Start Watchdog
    Wait For Plugin To Start  /tmp/TestWatchdogRestartsChildAfterRestartExit  30
    ## Then waits 10 seconds and restarts
    Wait For Plugin To Start Again  /tmp/TestWatchdogRestartsChildAfterRestartExit  3


*** Keywords ***