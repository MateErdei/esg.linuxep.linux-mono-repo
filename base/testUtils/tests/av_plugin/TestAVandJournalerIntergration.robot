*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  AVResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource    ../edr_plugin/EDRResources.robot
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Resource  ../mcs_router/McsRouterResources.robot
Suite Setup     Setup For Fake Cloud
Suite Teardown  Require Uninstalled

Test Teardown  Run Keywords
...            Run Keyword If Test Failed  Dump Teardown Log  /tmp/av_install.log  AND
...            Remove File  /tmp/av_install.log  AND
...            Remove File  ${EVENT_JOURNAL_DIR}/SophosSPL/Detections/*  AND
...            Stop Local Cloud Server  AND
...            General Test Teardown

Force Tags  LOAD4
Default Tags   EVENT_JOURNALER_PLUGIN   AV_PLUGIN   EDR_PLUGIN


*** Test Cases ***
Test av can publish events and that journaler can receive them
    [Timeout]  10 minutes
    Setup Event Journaler End To End

    Check Journal Is Empty
    Mark Livequery Log  False

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  2 secs
        ...  Check Logs Detected EICAR Event  1

Test av can publish events and that journaler can receive them after av restart
    [Timeout]  10 minutes
    Setup Event Journaler End To End

    Check Journal Is Empty
    Mark Livequery Log  False

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  1

    Stop AV Plugin
    Start AV Plugin

    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  2

Test av can publish events and that journaler can receive them after event journaler restart
    [Timeout]  10 minutes
    Setup Event Journaler End To End

    Check Journal Is Empty
    Mark Livequery Log  False

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  1

    Restart Event Journaler

    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  2

Test av can publish events and that journaler can receive them after edr restart
    [Timeout]  10 minutes
    Setup Event Journaler End To End

    Check Journal Is Empty
    Mark Livequery Log  False

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  1

    Stop EDR Plugin
    Start EDR Plugin

    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  2


*** Keywords ***
Setup For Fake Cloud
    Regenerate Certificates
    Require Fresh Install
    Set Local CA Environment Variable
    Override LogConf File as Global Level  DEBUG
    Check For Existing MCSRouter
    Cleanup MCSRouter Directories
    Cleanup Local Cloud Server Logs

Setup Event Journaler End To End
    Start Local Cloud Server
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud
    Install EDR Directly
    Install Event Journaler Directly
    Install AV Plugin Directly

Check Logs Detected EICAR Event
    [Arguments]  ${EXPECTED_EICARS}
    ${TWO_PER_EICAR} =  Evaluate  ${EXPECTED_EICARS}*2
    ${FOUR_PER_EICAR} =   Evaluate  ${EXPECTED_EICARS}*4
    Check Marked Livequery Log Contains String N Times   dirty_file         ${FOUR_PER_EICAR}
    Check Marked Livequery Log Contains String N Times   sha256FileHash     ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   "EICAR-AV-Test",   ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   "primary\\":true   ${TWO_PER_EICAR}
    Check Marked Livequery Log Contains String N Times   threatSource       ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   threatType         ${EXPECTED_EICARS}

Detect EICAR And Read With Livequery Via Event Journaler
    Check AV Plugin Can Scan Files
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Journal Contains Detection Event With Content  ${JOURNALED_EICAR}
    Run Live Query  select * from sophos_detections_journal   simple
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Marked Livequery Log Contains  Successfully executed query
