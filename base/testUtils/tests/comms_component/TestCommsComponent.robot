*** Settings ***

Resource  ../installer/InstallerResources.robot
Resource  CommsComponentResources.robot
Library   ${LIBS_DIRECTORY}/CommsComponentUtils.py
Library   ${LIBS_DIRECTORY}/LogUtils.py
Library   ${LIBS_DIRECTORY}/OSUtils.py

Default Tags  COMMS
Test Setup  Test Setup
Test Teardown  Test Teardown

*** Test Cases ***

Test Comms Component Starts
    [Tags]   COMMS  TAP_TESTS
    Require Installed
    File Should Exist  ${SOPHOS_INSTALL}/base/bin/CommsComponent

    Check Watchdog Starts Comms Component
    Check Comms Component Log Does Not Contain Error

    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log  sophos-spl-local  sophos-spl-group  -rw-------
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log  sophos-spl-network  sophos-spl-group  -rw-------


*** Keywords ***
Test Setup
    Require Uninstalled

Test Teardown
    General Test Teardown
    Require Uninstalled
