*** Settings ***
Documentation    Suite description

Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot

Library     ${COMMON_TEST_LIBS}/LiveQueryUtils.py

Suite Setup     Require Installed
Suite Teardown  Require Uninstalled

Test Teardown  General Test Teardown

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
