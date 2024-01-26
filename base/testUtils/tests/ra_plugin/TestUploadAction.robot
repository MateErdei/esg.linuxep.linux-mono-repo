*** Settings ***
Library     ${COMMON_TEST_LIBS}/MCSRouter.py
Library     ${COMMON_TEST_LIBS}/OnFail.py
Library     ${COMMON_TEST_LIBS}/CentralUtils.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Suite Setup     RA Action With Register Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup  RA Test Setup
Test Teardown  RA Test Teardown

Force Tags   RESPONSE_ACTIONS_PLUGIN  TAP_PARALLEL1

*** Test Cases ***
RA Plugin uploads a file successfully
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Upload_File_From_Fake_Cloud
    wait_for_log_contains_from_mark  ${response_mark}  Action correlation-id has succeeded   60
    wait_for_log_contains_from_mark  ${action_mark}  Sent upload response for ID correlation-id to Centra   15
    wait_for_log_contains_from_mark  ${action_mark}   Upload for /tmp/file succeeded

    Check Log Contains  Received HTTP PUT Request  ${HTTPS_LOG_FILE_PATH}  https server log
    Check Log Contains  tempfilecontent  ${HTTPS_LOG_FILE_PATH}  https server log
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"fileName\":\"file\",\"httpStatus\":200,\"result\":0,\"sha256\":\"382a1f7b76ca05728a4e9e6b3a4877f8634fb33b08decc5251933705a684f34f\"

RA Plugin runs actions in order
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [responseactions]\nVERBOSITY=DEBUG\n
    Restart Response Actions
    generate_file  /tmp/largefile  ${500}
    Register Cleanup  Remove File  /tmp/largefile
    Simulate Response Action  ${SUPPORT_FILES}/CentralXml/UploadAction.json
    Simulate Response Action  ${SUPPORT_FILES}/CentralXml/UploadAction.json    id2

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: id2

    Check Log Contains In Order    ${RESPONSE_ACTIONS_LOG_PATH}
    ...     Received new Action: id1
    ...     Received new Action: id2

    Check Log Contains In Order    ${RESPONSE_ACTIONS_LOG_PATH}
    ...     Running action: id1
    ...     Action id1 has succeeded
    ...     Running action: id2
    ...     Action id2 has succeeded



RA Plugin uploads a file successfully with compression
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Upload_File_From_Fake_Cloud   /tmp/file  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   60
    wait_for_log_contains_from_mark  ${action_mark}  Sent upload response for ID corrid to Centra   15

    File Should exist  /tmp/upload.zip
    Create Directory  /tmp/unpackzip/
    Register Cleanup  Remove Directory  /tmp/unpackzip/    recursive=${True}
    ${unzipTool} =  Set Variable  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/unzipTool
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
    Register Cleanup  Remove Directory  /tmp/compressionTest    recursive=${True}
    Send_Upload_Folder_From_Fake_Cloud   /tmp/compressionTest  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   60
    wait_for_log_contains_from_mark  ${action_mark}  Sent upload folder response for ID corrid to Central   15
    File Should exist  /tmp/upload.zip
    Create Directory  /tmp/unpackzip/
    Register Cleanup  Remove Directory  /tmp/unpackzip/    recursive=${True}
    ${unzipTool} =  Set Variable  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/unzipTool
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${unzipTool} /tmp/upload.zip /tmp/unpackzip/ password  shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    File Should exist  /tmp/unpackzip/compressionTest/file.txt
    File Should Contain  /tmp/unpackzip/compressionTest/file.txt     tempfilecontent