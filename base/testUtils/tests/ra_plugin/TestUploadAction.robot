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

Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Test Cases ***
RA Plugin uploads a file successfully
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [responseactions]\nVERBOSITY=DEBUG\n
    Install Response Actions Directly
    Create File  /tmp/file  tempfilecontent
    Send_Upload_File_From_Fake_Cloud
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  1 secs
    ...  Check Log Contains  Running upload  ${RESPONSE_ACTIONS_LOG_PATH}  response actions log
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Log Contains  Sent upload response to Central  ${RESPONSE_ACTIONS_LOG_PATH}  response actions log
    Check Log Contains  Upload for /tmp/file succeeded  ${RESPONSE_ACTIONS_LOG_PATH}  response actions log
    Check Log Contains  Received HTTP PUT Request  ${HTTPS_LOG_FILE_PATH}  https server log
    Check Log Contains  tempfilecontent  ${HTTPS_LOG_FILE_PATH}  https server log
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"fileName\":\"file\",\"httpStatus\":200,\"result\":0,\"sha256\":\"382a1f7b76ca05728a4e9e6b3a4877f8634fb33b08decc5251933705a684f34f\"
*** Keywords ***
RA Upload Suite Setup
    Start Local Cloud Server
    Regenerate Certificates
    Set Local CA Environment Variable
    Run Full Installer
    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server

RA Upload Test Setup
    Require Installed
    HttpsServer.Start Https Server  /tmp/cert.crt  443  tlsv1_2  True
    install_system_ca_cert  /tmp/cert.crt
    install_system_ca_cert  /tmp/ca.crt

RA Upload Test Teardown
    General Test Teardown
    Uninstall Response Actions
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    cleanup_system_ca_certs
    Remove File  ${EXE_CONFIG_FILE}
    Run Keyword If Test Failed    Dump Cloud Server Log
    Stop Local Cloud Server
