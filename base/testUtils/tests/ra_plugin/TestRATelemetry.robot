*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot

Suite Setup     RA Telemetry Suite Setup
Suite Teardown  Require Uninstalled

Test Setup  RA Telemetry Test Setup
Test Teardown  RA Telemetry Test Teardown

Default Tags   RESPONSE_ACTIONS_PLUGIN  TAP_TESTS


*** Test Cases ***
RA Plugin Reports Telemetry Correctly
    Install Response Actions Directly
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}

*** Keywords ***
RA Telemetry Suite Setup
    Require Fresh Install
    Copy Telemetry Config File in To Place
    Create File    ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [telemetry]\nVERBOSITY=DEBUG\n

RA Telemetry Test Setup
    Require Installed
    Prepare To Run Telemetry Executable

RA Telemetry Test Teardown
    General Test Teardown
    Uninstall Response Actions
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove File  ${EXE_CONFIG_FILE}
