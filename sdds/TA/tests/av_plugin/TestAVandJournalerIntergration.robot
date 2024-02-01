*** Settings ***
Documentation    Suite description

Resource  ${COMMON_TEST_ROBOT}/AVResources.robot
Resource  ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource  ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource  ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource  ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource  ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Library   ${COMMON_TEST_LIBS}/FileUtils.py
Library   ${COMMON_TEST_LIBS}/LiveQueryUtils.py
Library   ${COMMON_TEST_LIBS}/LogUtils.py
Library   ${COMMON_TEST_LIBS}/OnFail.py

Suite Setup     AV System Suite Setup
Suite Teardown  AV System Suite Teardown

Test Setup     AV System Test Setup
Test Teardown  AV System Test Teardown


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
    ${av_mark} =    mark_log_size   ${SOPHOS_INSTALL}/plugins/av/log/av.log
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    wait_for_log_contains_from_mark    ${av_mark}    Processing On Access Scanning settings from CORE policy     ${20}
    wait_for_file_to_contain  ${ONACCESS_STATUS_FILE}  enabled  timeout=${20}

    # File needs to be detected by on-access
    ${av_mark} =    mark_log_size   ${SOPHOS_INSTALL}/plugins/av/log/av.log
    Create File     /tmp/dirty_included_file    ${EICAR_STRING}
    wait_for_log_contains_from_mark    ${av_mark}    Threat cleaned up at path: '/tmp/dirty_included_file'     20

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
AV System Test Setup
    Register Cleanup    Remove File  /tmp/av_install.log
    ${relevant_log_list} =    Create List    plugins/eventjournaler/log/*.log*  plugins/av/log/*.log*  plugins/av/log/sophos_threat_detector/*.log*
    Register Cleanup    Check Logs Do Not Contain Error    ${relevant_log_list}
    Register On Fail    dump_cloud_server_log
    Register On Fail    Dump Teardown Log  /tmp/av_install.log

AV System Test Teardown
    Run Teardown Functions
    Stop AV Plugin
    Remove File  ${SOPHOS_INSTALL}/plugins/av/var/persist-threatDatabase
    Start AV Plugin
    Check AV Plugin Installed Directly
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_network_tables_disabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Remove File  ${EVENT_JOURNAL_DIR}/SophosSPL/Detections/*
    General Test Teardown

AV System Suite Setup
    Setup For Fake Cloud
    Setup Event Journaler End To End

AV System Suite Teardown
    Dump Cloud Server Log
    Require Uninstalled
    Stop Local Cloud Server

Wait Until Threat Report Socket Exists
    [Arguments]    ${time_to_wait}=5
    Wait Until Keyword Succeeds
    ...  ${time_to_wait} secs
    ...  1 secs
    ...  Should Exist    ${THREAT_REPORT_SOCKET_PATH}

Setup For Fake Cloud
    Require Fresh Install
    Setup_MCS_Cert_Override
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
    Check Marked Livequery Log Contains String N Times   quarantineSuccess\\":true  ${EXPECTED_EICARS}

Detect EICAR And Read With Livequery Via Event Journaler
    # dirty_file is scanned with avscanner, so we don't want on-access to detect it
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
