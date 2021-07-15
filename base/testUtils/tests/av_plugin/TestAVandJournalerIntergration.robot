*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  AVResources.robot
Resource  ../watchdog/WatchdogResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Teardown  General Test Teardown

Default Tags   EVENT_JOURNALER_PLUGIN   AV_PLUGIN


*** Test Cases ***
Test av can publish events and that journaler can recieve them
    Install Event Journaler Directly
    Override LogConf File as Global Level  DEBUG
    Install AV Plugin Directly
    Check AV Plugin Can Scan Files
    sleep  20
    fail