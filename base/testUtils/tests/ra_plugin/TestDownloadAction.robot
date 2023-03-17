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

Test Setup  RA Test Setup
Test Teardown  RA Test Teardown

Force Tags  LOAD5
Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Variables ***
${DOWNLOAD_TARGET_PATH}    /tmp/folder    #if this changes change in Send_Download_File_From_Fake_Cloud also
${DOWNLOAD_FILENAME}    download.zip     #if this changes change in httpserver to

*** Test Cases ***
RA Plugin downloads a file successfully
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Send_Download_File_From_Fake_Cloud
    wait_for_log_contains_from_mark  ${response_mark}  Action correlation-id has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent download response for id correlation-id to Central   15
    wait_for_log_contains_from_mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}/${DOWNLOAD_FILENAME} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   /tmp/folder/download.zip
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}/download.zip

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"type\":sophos.mgt.response.DownloadFile,\"httpStatus\":200,\"result\":0


RA Plugin downloads a file successfully with decompression
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Send_Download_File_From_Fake_Cloud   ${TRUE}
    wait_for_log_contains_from_mark  ${response_mark}  Action correlation-id has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent download response for id correlation-id to Central   15
    wait_for_log_contains_from_mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}/${DOWNLOAD_FILENAME} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   /tmp/folder/download.zip
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}/download.zip
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}/extract/download.txt

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"type\":sophos.mgt.response.DownloadFile,\"httpStatus\":200,\"result\":0


RA Plugin downloads a file successfully with password protected decompression
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Download_File_From_Fake_Cloud   /tmp/file  ${TRUE}  corrid  password
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

RA Upload Action Suite Setup
    RA Suite Setup
    Register With Local Cloud Server
