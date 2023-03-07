*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OnFail.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Suite Setup     RA Upload Action Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup  RA Upload Test Setup
Test Teardown  RA Upload Test Teardown

Force Tags  LOAD5
Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Test Cases ***
RA Plugin uploads a file successfully
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
    Restart Response Actions
    generate_file  /tmp/largefile  500
    Register Cleanup  Remove File  /tmp/largefile
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

RA Plugin uploads a folder successfully with compression
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create Directory  /tmp/compressionTest
    Create File  /tmp/compressionTest/file.txt  tempfilecontent
    Register Cleanup  Remove Directory  /tmp/compressionTest
    Send_Upload_Folder_From_Fake_Cloud   /tmp/compressionTest  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent upload folder response for id corrid to Central   15
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

RA Upload Action Suite Setup
    RA Upload Suite Setup
    Register With Local Cloud Server
