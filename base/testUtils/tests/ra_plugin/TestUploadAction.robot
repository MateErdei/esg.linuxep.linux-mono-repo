*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Suite Setup     RA Upload Suite Setup
Suite Teardown  Require Uninstalled

Test Setup  RA Upload Test Setup
Test Teardown  RA Upload Test Teardown

Default Tags   RESPONSE_ACTIONS_PLUGIN  TAP_TESTS


*** Test Cases ***
RA Plugin Can do Upload File
    Install Response Actions Directly
    Send_Upload_File_From_Fake_Cloud
    create File  /tmp/file
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  Running upload  ${RESPONSE_ACTIONS_LOG_PATH}  response actions log
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  Sent upload response to Central ${RESPONSE_ACTIONS_LOG_PATH}  response actions log
*** Keywords ***
RA Upload Suite Setup
    Start Local Cloud Server
    Regenerate Certificates
    Set Local CA Environment Variable
    Run Full Installer
    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Fake Cloud

RA Upload Test Setup
    Require Installed
    Prepare To Run Telemetry Executable

RA Upload Test Teardown
    General Test Teardown
    Uninstall Response Actions
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove File  ${EXE_CONFIG_FILE}
    Run Keyword If Test Failed    Dump Cloud Server Log
    Stop Local Cloud Server
