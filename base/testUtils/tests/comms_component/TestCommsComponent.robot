*** Settings ***

Resource  ../installer/InstallerResources.robot
Resource  CommsComponentResources.robot
Library   ${LIBS_DIRECTORY}/CommsComponentUtils.py
Library   ${LIBS_DIRECTORY}/LogUtils.py

Default Tags  COMMS
Test Setup  Test Setup
Test Teardown  Test Teardown

*** Test Cases ***

Test Comms Component Starts
    [Tags]   COMMS  TAP_TESTS
    Require Installed
    File Should Exist  ${SOPHOS_INSTALL}/base/bin/CommsComponent
    Check Watchdog Starts Comms Component


*** Keywords ***
Test Setup
    Require Uninstalled

Test Teardown
    General Test Teardown
    Require Uninstalled
