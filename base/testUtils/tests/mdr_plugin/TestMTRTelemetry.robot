*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LogUtils.py


Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  ../comms_component/CommsComponentResources.robot
Resource  MDRResources.robot


Test Setup  MTR Telemetry Test Setup
Test Teardown  MTR Telemetry Test Teardown

Suite Setup   MTR Telemetry Suite Setup
Suite Teardown   MTR Telemetry Suite Teardown

Default Tags   MDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
Force Tags  LOAD6

*** Test Cases ***
MTR Plugin Reports Telemetry Correctly With A SophosMTR Restart And Also Uses Cached Values From Disk
    Install MTR From Fake Component Suite
    Kill SophosMTR Executable
    Wait Until SophosMTR Executable Running  20

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  1

    Stop MDR Plugin
    Start MDR Plugin

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  0


MTR Plugin Counts SophosMTR Restarts Correctly And Reports In Telemetry
    Install MTR From Fake Component Suite
    # the server seems to die if it doesn't recieve any input for a while
    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}

    Kill SophosMTR Executable
    Wait Until SophosMTR Executable Running  20

    Kill SophosMTR Executable
    Wait Until SophosMTR Executable Running  20

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check MTR Telemetry Json Is Correct  ${telemetryFileContents}  2

*** Keywords ***
MTR Telemetry Suite Setup
    Require Fresh Install
    Copy Telemetry Config File in To Place
    Create File    ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [comms_network]\nVERBOSITY=DEBUG\n[comms_component]\nVERBOSITY=DEBUG\n[telemetry]\nVERBOSITY=DEBUG\n
    Restart Comms

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