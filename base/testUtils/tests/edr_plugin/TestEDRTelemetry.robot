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

Default Tags   EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY

*** Variables ***
${COMPONENT_TEMP_DIR}  /tmp/edr_component
${CRASH_QUERY} =  WITH RECURSIVE counting (curr, next) AS ( SELECT 1,1 UNION ALL SELECT next, curr+1 FROM counting LIMIT 10000000000 ) SELECT group_concat(curr) FROM counting;


*** Test Cases ***
EDR Plugin Produces Telemetry With OSQueryD Output Log File Not Containing Restarts
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  0

EDR Plugin Counts OSQuery Restarts Correctly And Reports In Telemetry
    Wait Until OSQuery Running  20
    Kill OSQuery
    Wait Until OSQuery Running  20
    Kill OSQuery
    Wait Until OSQuery Running  20

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  2  0  0  0

EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts
    Run Live Query  ${CRASH_QUERY}  Crash

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check EDR Log Contains    Extension exited while running

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  1  0

EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts And Restarts by EDR Plugin
    Wait Until OSQuery Running  20
    Kill OSQuery
    Wait Until OSQuery Running  20
    Kill OSQuery
    Wait Until OSQuery Running  20

    Run Live Query  ${CRASH_QUERY}  Crash
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check EDR Log Contains    Extension exited while running

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  2  0  1  0

EDR Plugin Counts Osquery Database Purges
    Prepare To Run Telemetry Executable
    Trigger EDR Osquery Database Purge
    Restart EDR Plugin
    Wait For EDR Osquery To Purge Database
    ${file_number} =  Count Files In Directory  /opt/sophos-spl/plugins/edr/var/osquery.db/
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  1  1  0  0  ignore_cpu_restarts=True  ignore_memory_restarts=True

*** Keywords ***
EDR Telemetry Suite Setup
    Require Fresh Install
    Install EDR Directly
    Copy Telemetry Config File in To Place


EDR Telemetry Suite Teardown
    Uninstall SSPL

EDR Telemetry Test Setup
    Require Installed
    Restart EDR Plugin  True
    Create Directory   ${COMPONENT_TEMP_DIR}
    Wait Until OSQuery Running  20

EDR Telemetry Test Teardown
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    General Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove Directory   ${COMPONENT_TEMP_DIR}  recursive=true
    Remove File  ${EXE_CONFIG_FILE}

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