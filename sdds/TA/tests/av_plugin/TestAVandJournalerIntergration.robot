*** Settings ***
Documentation    Suite description

Resource  ${COMMON_TEST_ROBOT}/AVResources.robot
Resource  ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource  ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource  ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource  ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource  ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Library   ${COMMON_TEST_LIBS}/LiveQueryUtils.py
Library   ${COMMON_TEST_LIBS}/LogUtils.py

Suite Setup     Run keywords
...             Setup For Fake Cloud  AND
...             Setup Event Journaler End To End

Suite Teardown  Run Keywords
...             dump_cloud_server_log  AND
...             Require Uninstalled  AND
...             Stop Local Cloud Server

Test Teardown  Run Keywords
...            Run Keyword If Test Failed  Dump Teardown Log  /tmp/av_install.log  AND
...            dump_cloud_server_log  AND
...            Remove File  /tmp/av_install.log  AND
...            Stop AV Plugin  AND
...            Remove File  ${SOPHOS_INSTALL}/plugins/av/var/persist-threatDatabase  AND
...            Start AV Plugin  AND
...            Check AV Plugin Installed Directly  AND
...            Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_network_tables_disabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  AND
...            Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  AND
...            Remove File  ${EVENT_JOURNAL_DIR}/SophosSPL/Detections/*  AND
...            General Test Teardown

Force Tags  TAP_PARALLEL2  EVENT_JOURNALER_PLUGIN  AV_PLUGIN  EDR_PLUGIN

*** Test Cases ***
Test av can publish events and that journaler can receive them
    [Timeout]  10 minutes
    Check Journal Is Empty
    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  1

Test av can publish events and that journaler can receive them after av restart
    [Timeout]  10 minutes
    Check Journal Is Empty
    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  1

    Mark Sophos Threat Detector Log

    Stop AV Plugin
    Start AV Plugin
    Wait Until Threat Report Socket Exists

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check AV Plugin Running

    Mark Livequery Log

    Create File     /tmp/dirty_excluded_file2    ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLS_PATH} /tmp/dirty_excluded_file2
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Journal Contains X Detection Events  2
    Run Live Query  select * from sophos_detections_journal   simple
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check Marked Livequery Log Contains  Successfully executed query

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  2

Test av can publish events and that journaler can receive them after event journaler restart
    [Timeout]  10 minutes
    Check Journal Is Empty
    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  1

    Restart Event Journaler

    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler  /tmp/dirty_excluded_file2  ${JOURNALED_EICAR2}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  2

Test av can publish events and that journaler can receive them after edr restart
    [Timeout]  10 minutes
    Check Journal Is Empty
    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  1

    Restart EDR Plugin

    Mark Livequery Log

    Detect EICAR And Read With Livequery Via Event Journaler   /tmp/dirty_excluded_file2  ${JOURNALED_EICAR2}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Logs Detected EICAR Event  2

Test av can publish events for onaccess and that journaler can receive them
    [Timeout]  10 minutes
    Check Journal Is Empty
    Mark Livequery Log
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_onaccess_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  AV Plugin Log Contains   Processing On Access Scanning settings from CORE policy
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 secs
    ...  AV Plugin Log Contains   On-access is enabled in the FLAGS policy, assuming on-access policy settings
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  check_log_contains    On-access enabled: true  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log    soapd
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  check_log_contains    Fanotify successfully initialised  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log    soapd

    Create File     /tmp/dirty_excluded_file    ${EICAR_STRING}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  check_log_contains  Threat health changed to suspicious   ${AV_LOG_FILE}    av
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Journal Contains Detection Event With Content  "avScanType":201
    Check Journal Contains Detection Event With Content  pid
    Check Journal Contains Detection Event With Content  processPath
    Run Live Query  select * from sophos_detections_journal   simple
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check Marked Livequery Log Contains  Successfully executed query
    Check Marked Livequery Log Contains  pid
    Check Marked Livequery Log Contains  processPath
    Check Marked Livequery Log Contains  process_path
    Check Marked Livequery Log Contains  on_access
*** Keywords ***
Wait Until Threat Report Socket Exists
    [Arguments]    ${time_to_wait}=5
    Wait Until Keyword Succeeds
    ...  ${time_to_wait} secs
    ...  1 secs
    ...  Should Exist    ${THREAT_REPORT_SOCKET_PATH}

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
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_network_tables_disabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud
    Install EDR Directly
    Create Query Packs
    Restart EDR Plugin
    Register Cleanup   Cleanup Query Packs
    Install Event Journaler Directly
    Install AV Plugin Directly

Check Logs Detected EICAR Event
    [Arguments]  ${EXPECTED_EICARS}
    ${TWO_PER_EICAR} =  Evaluate  ${EXPECTED_EICARS}*2
    ${FOUR_PER_EICAR} =   Evaluate  ${EXPECTED_EICARS}*4
    Check Marked Livequery Log Contains String N Times   dirty_excluded_file        ${FOUR_PER_EICAR}
    Check Marked Livequery Log Contains String N Times   sha256FileHash             ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   "EICAR-AV-Test",           ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   "primary\\":true           ${TWO_PER_EICAR}
    Check Marked Livequery Log Contains String N Times   threatSource               ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   threatType                 ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   av_scan_type               1
    Check Marked Livequery Log Contains String N Times   quarantine_success         1
    Check Marked Livequery Log Contains String N Times   on_demand                  ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   avScanType\\":203          ${EXPECTED_EICARS}
    Check Marked Livequery Log Contains String N Times   quarantineSuccess\\":false  ${EXPECTED_EICARS}

Detect EICAR And Read With Livequery Via Event Journaler
    [Arguments]  ${dirty_file}=/tmp/dirty_excluded_file   ${journal_string}=${JOURNALED_EICAR}
    Check AV Plugin Can Scan Files  ${dirty_file}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Journal Contains Detection Event With Content  ${journal_string}
    Run Live Query  select * from sophos_detections_journal   simple
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check Marked Livequery Log Contains  Successfully executed query
