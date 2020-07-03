*** Settings ***

Resource  ../installer/InstallerResources.robot
Resource  CommsComponentResources.robot
Library   ${LIBS_DIRECTORY}/CommsComponentUtils.py
Library   ${LIBS_DIRECTORY}/LogUtils.py

Default Tags  COMMS
Test Setup  Require Uninstalled

Suite Teardown  Require Uninstalled
Suite Setup  Require Uninstalled

*** Test Cases ***

Test Comms Component Starts
    Require Fresh Install
    File Should Exist  ${SOPHOS_INSTALL}/base/bin/CommsComponent
    Check Watchdog Starts Comms Component
