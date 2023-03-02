*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OnFail.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Suite Setup     RA Upload Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup  RA Upload Test Setup
Test Teardown  RA Upload Test Teardown

Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Test Cases ***
RA Plugin uploads a file successfully
    Install Response Actions Directly
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Upload_File_From_Fake_Cloud
    wait_for_log_contains_from_mark  ${response_mark}  Action correlation-id has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent upload response for id correlation-id to Centra   15
    wait_for_log_contains_from_mark  ${action_mark}   Upload for /tmp/file succeeded

    Check Log Contains  Received HTTP PUT Request  ${HTTPS_LOG_FILE_PATH}  https server log
    Check Log Contains  tempfilecontent  ${HTTPS_LOG_FILE_PATH}  https server log
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"fileName\":\"file\",\"httpStatus\":200,\"result\":0,\"sha256\":\"382a1f7b76ca05728a4e9e6b3a4877f8634fb33b08decc5251933705a684f34f\"

RA Plugin runs actions in order
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [responseactions]\nVERBOSITY=DEBUG\n
    Install Response Actions Directly
    generate_file  /tmp/file  500
    Register Cleanup  Remove File  /tmp/file
    Simulate Upload Action Now
    Simulate Upload Action Now  id2

    Wait Until Keyword Succeeds
    ...  80 secs
    ...  10 secs
    ...  Check Log Contains In Order   ${RESPONSE_ACTIONS_LOG_PATH}
        ...  Received new Action
        ...  Received new Action
        ...  Action id1 has succeeded
        ...  Running action: id2
        ...  Action id2 has succeeded

RA Plugin uploads a file successfully with compression
    Install Response Actions Directly
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Upload_File_From_Fake_Cloud   /tmp/file  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent upload response for id corrid to Centra   15

    File Should exist  /tmp/upload.zip
    Create Directory  /tmp/unpackzip/
    Register Cleanup  Remove Directory  /tmp/unpackzip/
    ${unzipTool} =  Set Variable  SystemProductTestOutput/unzipTool
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${unzipTool} /tmp/upload.zip /tmp/unpackzip/ password  shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    File Should exist  /tmp/unpackzip/file
    File Should Contain  /tmp/unpackzip/file     tempfilecontent

*** Keywords ***
Simulate Upload Action Now
    [Arguments]  ${id_suffix}=id1
    Copy File   ${SUPPORT_FILES}/CentralXml/UploadAction.json  ${SOPHOS_INSTALL}/tmp
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${SOPHOS_INSTALL}/tmp/UploadAction.json    shell=True
    Should Be Equal As Integers    ${result.rc}    0  Failed to replace permission to file. Reason: ${result.stderr}
    Move File   ${SOPHOS_INSTALL}/tmp/UploadAction.json  ${SOPHOS_INSTALL}/base/mcs/action/CORE_${id_suffix}_request_2030-02-27T13:45:35.699544Z_144444000000004.json

RA Suite Teardown
    Stop Local Cloud Server
    Require Uninstalled

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
    Remove File   /tmp/upload.zip
