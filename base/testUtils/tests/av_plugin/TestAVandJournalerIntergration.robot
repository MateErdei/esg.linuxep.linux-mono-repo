*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  AVResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource    ../edr_plugin/EDRResources.robot
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py
Resource  ../mcs_router/McsRouterResources.robot
Suite Setup     Setup For Fake Cloud
Suite Teardown  Require Uninstalled

Test Teardown  Run Keywords
...            Run Keyword If Test Failed  Dump Teardown Log  /tmp/install.sh  AND
...            Remove File  /tmp/install.sh  AND
...            General Test Teardown

Force Tags  LOAD4
Default Tags   EVENT_JOURNALER_PLUGIN   AV_PLUGIN


*** Test Cases ***
Test av can publish events and that journaler can recieve them
    [Timeout]  10 minutes
    Start Local Cloud Server
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud
    Install EDR Directly
    Install Event Journaler Directly
    Install AV Plugin Directly
    Run Keyword And Expect Error  Event read process failed with: 1: 1 != 0   Check Journal Contains Detection Event With Content  ${JOURNALED_EICAR}

    Check AV Plugin Can Scan Files

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Journal Contains Detection Event With Content  ${JOURNALED_EICAR}

    Run Live Query  select * from sophos_detections_journal   simple

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query  1

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Log Contains   /tmp/dirty_file    ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log
    Check Log Contains   /tmp/dirty_file    ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log
    Check Log Contains   sha256FileHash     ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log
    Check Log Contains   EICAR-AV-Test      ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log
    Check Log Contains   "primary\\":true    ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log
    Check Log Contains   threatSource       ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log
    Check Log Contains   threatType         ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   livequery_log

*** Keywords ***
Setup For Fake Cloud
    Regenerate Certificates
    Require Fresh Install
    Set Local CA Environment Variable
    Override LogConf File as Global Level  DEBUG
    Check For Existing MCSRouter
    Cleanup MCSRouter Directories
    Cleanup Local Cloud Server Logs
