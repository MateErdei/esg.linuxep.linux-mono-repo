*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py


Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  EDRResources.robot
Resource  ../upgrade_product/UpgradeResources.robot
Resource  ../mdr_plugin/MDRResources.robot


Test Setup  EDR Telemetry Test Setup
Test Teardown  EDR Telemetry Test Teardown

Suite Setup   EDR Telemetry Suite Setup
Suite Teardown   EDR Telemetry Suite Teardown

Default Tags   EDR_PLUGIN  TELEMETRY

*** Variables ***
${COMPONENT_TEMP_DIR}  /tmp/edr_component
${CRASH_QUERY} =  WITH RECURSIVE counting (curr, next) AS ( SELECT 1,1 UNION ALL SELECT next, curr+1 FROM counting LIMIT 10000000000 ) SELECT group_concat(curr) FROM counting;
${SIMPLE_QUERY_1_ROW} =  SELECT * from users limit 1;
${SIMPLE_QUERY_2_ROW} =  SELECT * from users limit 2;
${SIMPLE_QUERY_4_ROW} =  SELECT * from users limit 4;


*** Test Cases ***
EDR Plugin Produces Telemetry When XDR is enabled
    [Tags]  MCSROUTER  FAKE_CLOUD  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
    [Setup]  EDR Telemetry Test Setup With Cloud
    [Teardown]  EDR Telemetry Test Teardown With Cloud
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check EDR Log Contains  Flags have changed so restarting osquery
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check EDR Log Contains  Process task OSQUERYPROCESSFINISHED
    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  1  0  0  0  True  ignore_xdr=False

EDR Plugin Produces Telemetry For XDR scheduled queries
    [Tags]  MCSROUTER  FAKE_CLOUD  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
    [Setup]  EDR Telemetry Test Setup With Cloud
    [Teardown]  EDR Telemetry Test Teardown With Cloud
    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check EDR Log Contains  Flags have changed so restarting osquery

    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check EDR Log Contains  Error executing scheduled query bad-query:

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   Check EDR Log Contains  Executing scheduled query endpoint_id:
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    ${query1}=  Set Variable  {"name":"bad-query" ,"query-error-count": 1}
    ${query2}=  Set Variable  {"name":"endpoint_id" ,"record-size-std-deviation":0.0,"records-count":1}
    @{queries}=  create list   ${query1}  ${query2}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  1  0  0  0  True  ignore_xdr=False  ignore_scheduled_queries=False  scheduled_queries=@{queries}

EDR Plugin Produces Telemetry With OSQueryD Output Log File Not Containing Restarts
    Prepare To Run Telemetry Executable
    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  0

EDR Plugin Counts OSQuery Restarts Correctly And Reports In Telemetry
    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists

    Kill OSQuery
    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists
    Restart EDR Plugin              #Check telemetry persists after restart

    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists

    Kill OSQuery
    Wait Until OSQuery Running  20

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  2  0  0  0

EDR Plugin Counts OSQuery Restarts Correctly when XDR is enabled And Reports In Telemetry
    [Tags]  MCSROUTER  FAKE_CLOUD  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
    [Setup]  EDR Telemetry Test Setup With Cloud
    [Teardown]  EDR Telemetry Test Teardown With Cloud
    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check Log Contains In Order
            ...  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
            ...  Flags have changed so restarting osquery
    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists

    Kill OSQuery
    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists
    Restart EDR Plugin              #Check telemetry persists after restart

    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists

    Kill OSQuery
    Wait Until OSQuery Running  20

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  3  0  0  0


EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts
    Run Live Query  ${CRASH_QUERY}  Crash

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Livequery Log Contains    Extension exited while running

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    ${query}=  Set Variable  {"name":"Crash", "failed-osquery-died-count":1}
    @{queries}=  create list   ${query}

    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  1  0  queries=@{queries}


EDR Reports Telemetry And Stats Correctly After Plugin Restart For Live Query
    Run Live Query  ${SIMPLE_QUERY_1_ROW}   simple
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query with name: simple  1

    Restart EDR Plugin              #Check telemetry persists after restart

    Run Live Query  ${SIMPLE_QUERY_4_ROW}   simple
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query with name: simple  2

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    # ignoring duration as it will vary too much to reliably test - it's covered in unit tests.
    ${query1}=  Set Variable  {"name":"simple", "rowcount-std-deviation":1.5,"rowcount-avg":2.5, "rowcount-min":1, "rowcount-max":4, "successful-count":2}
    @{queries}=  create list   ${query1}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  0  queries=@{queries}

EDR Reports Telemetry Correctly When events max limit is hit for a table

    ${script} =     Catenate    SEPARATOR=\n
    ...    \Expiring events for subscriber: user_events (overflowed limit 100000)
    ...   Expiring events for subscriber: socket_events (overflowed limit 100000)
    ...   Expiring events for subscriber: selinux_events (overflowed limit 100000)
    ...   Expiring events for subscriber: process_events (overflowed limit 100000)
    ...   Expiring events for subscriber: process_events (overflowed limit 100000)
    ...    \
    Create File    ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.INFO.20201117-051713.1056   content=${script}



    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  0  ignore_process_events=False  ignore_selinux_events=False  ignore_socket_events=False  ignore_user_events=False


EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts And Restarts by EDR Plugin
    Wait Until OSQuery Running  20
    # osquery will take longer to restart if it is killed before the socket is created
    Wait Until Osquery Socket Exists
    Kill OSQuery

    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists
    Kill OSQuery

    Wait Until OSQuery Running  20

    Run Live Query  ${CRASH_QUERY}  Crash
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Livequery Log Contains    Extension exited while running

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    ${query}=  Set Variable  {"name":"Crash", "failed-osquery-died-count":1, "osquery-restarts":2}
    @{queries}=  create list   ${query}

    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  2  0  1  0  queries=@{queries}

EDR Plugin Counts Osquery Database Purges
    Prepare To Run Telemetry Executable
    Trigger EDR Osquery Database Purge
    Restart EDR Plugin
    Wait For EDR Osquery To Purge Database
    ${file_number} =  Count Files In Directory  /opt/sophos-spl/plugins/edr/var/osquery.db/
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  1  0  0  ignore_cpu_restarts=True  ignore_memory_restarts=True

EDR Plugin Produces Telemetry With OSQuery Max Events Override Value
    Prepare To Run Telemetry Executable

    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=345678

    Restart EDR Plugin
    Wait Until OSQuery Running  20
    Wait Until Osquery Socket Exists

    ${LogContents} =  Get File  ${EDR_DIR}/etc/osquery.flags
    Should Contain  ${LogContents}  --events_max=345678

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  0  events_max=345678

*** Keywords ***
EDR Telemetry Suite Setup
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Copy Telemetry Config File in To Place


EDR Telemetry Suite Teardown
    Uninstall SSPL

EDR Telemetry Test Setup
    Require Installed
    Install EDR Directly
    Create Directory   ${COMPONENT_TEMP_DIR}
    Wait Until OSQuery Running  20

EDR Telemetry Test Setup With Cloud
    Start Local Cloud Server   --initial-alc-policy  ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
    EDR Telemetry Test Setup
    Regenerate Certificates
    Set Local CA Environment Variable

EDR Telemetry Test Teardown
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    General Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove Directory   ${COMPONENT_TEMP_DIR}  recursive=true
    Remove File  ${EXE_CONFIG_FILE}
    Uninstall EDR Plugin

EDR Telemetry Test Teardown With Cloud
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    MCSRouter Default Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove Directory   ${COMPONENT_TEMP_DIR}  recursive=true
    Remove File  ${EXE_CONFIG_FILE}
    Uninstall EDR Plugin
    Stop Local Cloud Server
    Unset CA Environment Variable
    Cleanup Certificates

Trigger EDR Osquery Database Purge
    Should Exist  /opt/sophos-spl/plugins/edr/var/osquery.db
    ${result} =  Run Process  dd if\=/dev/urandom bs\=1024 count\=200 | split -a 4 -b 1k - /opt/sophos-spl/plugins/edr/var/osquery.db/  shell=true
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Wait For EDR Osquery To Purge Database
    Wait Until Keyword Succeeds
    ...  120s
    ...  10s
    ...  Check EDR Log Contains   Purging Database

Wait Until Osquery Socket Exists
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/osquery.sock