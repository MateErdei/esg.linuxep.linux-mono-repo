*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  ../event_journaler/EventJournalerResources.robot

Resource    ../edr_plugin/EDRResources.robot
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py

Suite Setup     Require Installed
Suite Teardown  Require Uninstalled

Test Teardown  Run Keywords
...             Stop Local Cloud Server  AND
...             Run Keyword If Test Failed    Dump Cloud Server Log   AND
...             General Test Teardown

Force Tags  LOAD4
Default Tags   EVENT_JOURNALER_PLUGIN    EDR_PLUGIN


*** Test Cases ***
Test we can query empty event journal
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [livequery]\nVERBOSITY=DEBUG\n
    Install EDR Directly
    Install Event Journaler Directly

    Run Live Query  select * from sophos_detections_journal   simple

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query  1

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Log Contains   "errorCode":0    ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log
