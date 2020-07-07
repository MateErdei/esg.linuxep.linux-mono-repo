*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LogUtils.py


Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  MDRResources.robot


Test Setup  MTR Telemetry Test Setup
Test Teardown  MTR Telemetry Test Teardown

Suite Setup   MTR Telemetry Suite Setup
Suite Teardown   MTR Telemetry Suite Teardown

Default Tags   MDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY

*** Variables ***
${PURGE_DATABASE_FILE_CONTENT} =  {"caller":"osquery.go:186","component":"dbos","level":"info","msg":"Osquery database watcher purge completed","publisher":"sophos","timestamp":"2019-10-09T10:09:35.535726746Z"}
${OSQUERY_RESTART_MEMORY} =  Aug 08 22:14:48 osquery: osqueryd worker (15319) stopping: Memory limits exceeded: 272292000
${OSQUERY_RESTART_CPU} =  Aug 08 22:12:25 osquery: osqueryd worker (28137) stopping: Maximum sustainable CPU utilization limit exceeded: 12
${OSQUERY_WATCHER_LOG} =  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/logs/osquery.watcher.log
${OSQUERYD_OUTPUT_LOG} =  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/logs/osqueryd.output.log


*** Test Cases ***
MTR Plugin Produces Telemetry With OSQueryD Output Log File Not Containing Restarts
    Install MTR From Fake Component Suite
    Create File  ${OSQUERYD_OUTPUT_LOG}  This is not a cpu or memory restart message
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  0

MTR Plugin Produces Telemetry With Empty OSQueryD Output Log File
    Install MTR From Fake Component Suite
    Create File  ${OSQUERYD_OUTPUT_LOG}
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  0


MTR Plugin Reports Telemetry Correctly With A SophosMTR Restart and No Telemetry To Report
    Install MTR From Fake Component Suite

    Kill SophosMTR Executable
    Wait Until SophosMTR Executable Running  20

    #If the file doesn't exist this returns -1 which when passed into Check MTR Telemetry Json Is Correct
    #causes the cpu and memory restarts to not be added to expected telemetry
    ${Expected_Restarts} =  Check If We Expect Telemetry For Restarts  ${OSQUERYD_OUTPUT_LOG}
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Log File  ${OSQUERYD_OUTPUT_LOG}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  1  0  ${Expected_Restarts}  ${Expected_Restarts}

    Stop MDR Plugin
    Start MDR Plugin

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  ${Expected_Restarts}  ${Expected_Restarts}


MTR Plugin Reports Telemetry Correctly For OSQuery Memory Restarts And Also Uses Cached Values From Disk
    Create File  ${OSQUERYD_OUTPUT_LOG}  ${OSQUERY_RESTART_MEMORY}
    Install MTR From Fake Component Suite

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  1

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}

MTR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts And Also Uses Cached Values From Disk
    Create File  ${OSQUERYD_OUTPUT_LOG}  ${OSQUERY_RESTART_CPU}
    Install MTR From Fake Component Suite

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  1

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}

MTR Plugin Reports Telemetry Correctly For OSQuery CPU And Memory Restarts And Also Uses Cached Values From Disk
    Create File  ${OSQUERYD_OUTPUT_LOG}  ${OSQUERY_RESTART_CPU}/n${OSQUERY_RESTART_MEMORY}
    Install MTR From Fake Component Suite

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  1  1

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}

MTR Plugin Counts Osquery Database Purges
    [Setup]  MTR Telemetry Test Setup Without Preparing To Run Telemetry Executable
    Install MTR From Fake Component Suite

    Osquery Has Not Purged Database
    Trigger Osquery Database Purge
    Wait For Osquery To Purge Database

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    ${fileContent}=  Get File  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/osquery.flags
    Should Contain  ${fileContent}    --logger_min_status=1

    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0  1  0  0  ignore_cpu_restarts=True  ignore_memory_restarts=True

MTR Plugin Counts SophosMTR Restarts Correctly And Reports In Telemetry
    Install MTR From Fake Component Suite

    Kill SophosMTR Executable
    Wait Until SophosMTR Executable Running  20

    Kill SophosMTR Executable
    Wait Until SophosMTR Executable Running  20

    #If the file doesn't exist this returns -1 which when passed into Check MTR Telemetry Json Is Correct
    #causes the cpu and memory restarts to not be added to expected telemetry
    ${Expected_Restarts} =  Check If We Expect Telemetry For Restarts  ${OSQUERYD_OUTPUT_LOG}
    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  2  0  ${Expected_Restarts}  ${Expected_Restarts}

*** Keywords ***
MTR Telemetry Suite Setup
    Require Fresh Install
    Copy Telemetry Config File in To Place

MTR Telemetry Suite Teardown
    Uninstall SSPL

MTR Telemetry Test Setup Without Preparing To Run Telemetry Executable
    Require Installed
    Create Directory   ${COMPONENT_TEMP_DIR}

MTR Telemetry Test Setup
    MTR Telemetry Test Setup Without Preparing To Run Telemetry Executable
    Prepare To Run Telemetry Executable

MTR Telemetry Test Teardown
    MDR Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove Directory   ${COMPONENT_TEMP_DIR}  recursive=true
    Remove File  ${EXE_CONFIG_FILE}