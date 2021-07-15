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
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  Received event  ${EVENT_JOURNALER_LOG_PATH}  event journaler log
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  {"details":{"filePath":"/tmp/dirty_file","sha256FileHash":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"/tmp/dirty_file","primary":true,"sha256":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","type":1}},"threatSource":1,"threatType":1,"time":  ${EVENT_JOURNALER_LOG_PATH}  event journaler log
