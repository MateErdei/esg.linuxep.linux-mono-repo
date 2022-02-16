*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  EventJournalerResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Teardown  Run Keywords
...            General Test Teardown   AND
...            Uninstall Event Journaler

Default Tags   EVENT_JOURNALER_PLUGIN


*** Test Cases ***
Event Journaler Plugin Installs With Version Ini File
    Install Event Journaler Directly
    File Should Exist   ${SOPHOS_INSTALL}/plugins/eventjournaler/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/eventjournaler/VERSION.ini   EventJournaler

Verify That Event Journaler Logging Can Be Set Individually
    Install Event Journaler Directly
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [eventjournaler]\nVERBOSITY=DEBUG\n

    Restart Event Journaler
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  Logger eventjournaler configured for level: DEBUG  ${EVENT_JOURNALER_LOG_PATH}  event journaler log