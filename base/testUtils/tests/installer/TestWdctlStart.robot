*** Settings ***
Documentation    Test that wdctl start operation

Library    ${libs_directory}/FullInstallerUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

*** Test Cases ***
Test wdctl times out if watchdog not running
    [Tags]    WDCTL  WATCHDOG
    Require Fresh Install
    Stop Watchdog
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl  start  foobar  timeout=1min
    Should Not Be Equal As Integers    ${result.rc}    ${-15}   "wdctl didn't timeout itself - was killed by robot"
    Should Not Be Equal As Integers    ${result.rc}    ${0}   "wdctl didn't report any error for timeout"
