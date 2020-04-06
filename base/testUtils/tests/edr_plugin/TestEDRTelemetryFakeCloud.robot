*** Settings ***
Suite Setup      EDR Telemetry FakeCloud Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       EDR Telemetry FakeCloud Test Setup
Test Teardown    EDR Test Teardown

Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py

Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../GeneralTeardownResource.robot
Resource  ../telemetry/TelemetryResources.robot

Resource    EDRResources.robot

Default Tags   EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY  AMAZON_LINUX


*** Variables ***
${BaseAndEdrVUTPolicy}              ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${EDR_PLUGIN_PATH}                  ${SOPHOS_INSTALL}/plugins/edr

*** Test Cases ***
EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts FakeCloud
    Install EDR And Override LogConf
    Test LiveQuery With FakeCloud   Crash Query  ${CRASH_QUERY}
    Wait Until Keyword Succeeds
    ...  80 secs
    ...  4 secs
    ...  Check EDR Log Contains    Extension exited while running

    Run And Verify EDR Telemetry  {"name":"Crash Query", "failed-osquery-died-count":1}  0  1

EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts And Restarts by EDR Plugin
    Install EDR And Override LogConf
    Wait Until OSQuery Running  10
    Kill OSQuery
    Wait Until OSQuery Running  20
    Kill OSQuery
    Wait Until OSQuery Running  20

    Test LiveQuery With FakeCloud  Crash Query  ${CRASH_QUERY}
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check EDR Log Contains    Extension exited while running

    Run And Verify EDR Telemetry  {"name":"Crash Query", "failed-osquery-died-count":1, "osquery-restarts":2}  2  1

EDR Plugin Reports Telemetry Correctly For Live Query FakeCloud
    Install EDR And Override LogConf
    Test LiveQuery With FakeCloud  simple   ${SIMPLE_QUERY_1_ROW}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  4 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/edr.log   edr_log  Successfully executed query with name: simple  1

    Trigger Query From Fake Cloud   simple   ${SIMPLE_QUERY_4_ROW}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  4 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/edr.log   edr_log  Successfully executed query with name: simple  2

    Run And Verify EDR Telemetry  {"name":"simple", "rowcount-avg":2.5, "rowcount-min":1, "rowcount-max":4, "successful-count":2}  0  0

*** Keywords ***
Trigger Query From Fake Cloud
    [Arguments]  ${name}  ${query}
    ${correlation_id} =  Get Correlation Id
    Send Query From Fake Cloud      ${name}  ${query}   command_id=${correlation_id}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains   /commands/applications/MCS;ALC;AGENT;LiveQuery;APPSPROXY

Run And Verify EDR Telemetry
    [Arguments]  ${query_metadata}  ${osquery_crashes}=0  ${failed_osquery_died_count}=0
    Prepare To Run Telemetry Executable With HTTPS Protocol  8443
    Log File  ${EXE_CONFIG_FILE}
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     0
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    # ignoring duration as it will vary too much to reliably test - it's covered in unit tests.
    @{queries}=  create list   ${query_metadata}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  ${osquery_crashes}  0  ${failed_osquery_died_count}  0  queries=@{queries}

Install EDR And Override LogConf
    Install EDR  ${BaseAndEdrVUTPolicy}
    Wait Until OSQuery Running

    Run Shell Process   /opt/sophos-spl/bin/wdctl stop edr     OnError=Failed to stop edr
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start edr    OnError=Failed to start edr

Test LiveQuery With FakeCloud
    [Arguments]  ${query_name}  ${query}
    Trigger Query From Fake Cloud   ${query_name}  ${query}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains       ${query_name}

EDR Telemetry FakeCloud Suite Setup
    EDR Suite Setup
    Copy Telemetry Config File in To Place

EDR Telemetry FakeCloud Test Setup
    EDR Test Setup
    Create Directory   ${COMPONENT_TEMP_DIR}

EDR Telemetry Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove Directory   ${COMPONENT_TEMP_DIR}  recursive=true
    Remove File  ${EXE_CONFIG_FILE}
    EDR Test Teardown