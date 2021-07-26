*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  AVResources.robot
Resource  ../watchdog/WatchdogResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Teardown  Run Keywords
...            Run Keyword If Test Failed  Dump Teardown Log  /tmp/install.sh  AND
...            Remove File  /tmp/install.sh  AND
...            General Test Teardown

Force Tags  LOAD4
Default Tags   EVENT_JOURNALER_PLUGIN   AV_PLUGIN


*** Test Cases ***
Test av can publish events and that journaler can recieve them
    Install Event Journaler Directly
    Install AV Plugin Directly
    Run Keyword And Expect Error  Event read process failed with: 1: 1 != 0   Check Journal Contains Detection Event With Content  ${JOURNALED_EICAR}

    Check AV Plugin Can Scan Files

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Journal Contains Detection Event With Content  ${JOURNALED_EICAR}
