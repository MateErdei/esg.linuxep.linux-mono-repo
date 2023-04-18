*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OnFail.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Suite Setup     RA Action With Register Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup  RA Test Setup
Test Teardown  RA Test Teardown

Force Tags  LOAD5
Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Variables ***
#if these change, change in Send_Download_File_From_Fake_Cloud also
${RESPONSE_ACTIONS_TMP_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/tmp/
${DOWNLOAD_TARGET_PATH}    /tmp/folder/
${DOWNLOAD_FILENAME_ZIP}    download.zip
${DOWNLOAD_FILENAME_TXT}    download.txt

*** Test Cases ***
RA Plugin downloads a file successfully
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}

    Send_Download_File_From_Fake_Cloud

    wait_for_log_contains_from_mark  ${response_mark}  Action correlation-id has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    wait_for_log_contains_from_mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP}

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":0


RA Plugin downloads a file successfully with decompression
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory  /tmp/folder  recursive=${TRUE}

    Send_Download_File_From_Fake_Cloud   ${TRUE}  ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_TXT}

    wait_for_log_contains_from_mark  ${response_mark}  Action correlation-id has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    wait_for_log_contains_from_mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_TXT} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_TXT}
    File Should Not Exist    ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP}

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}extract/${DOWNLOAD_FILENAME_TXT}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"httpStatus\":200,\"result\":0

RA Plugin fails a real url due to authorization not file to long
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    /tmp/folder/  recursive=${TRUE}

    Send_Download_File_From_Fake_Cloud_RealURL

    wait_for_log_contains_from_mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    wait_for_log_contains_from_mark  ${action_mark}   Failed to download, Error code: 403

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}download.zip

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"errorType\":\"network_error\",\"httpStatus\":403,\"result\":1

RA Plugin downloads a file successfully with password protected decompression
    [Tags]    TESTFAILURE
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Download_File_From_Fake_Cloud   /tmp/file  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent upload response for id corrid to Centra   15

    File Should exist  /tmp/upload.zip
    Create Directory  /tmp/unpackzip/
    Register Cleanup  Remove Directory  /tmp/unpackzip/    recursive=${True}
    ${unzipTool} =  Set Variable  SystemProductTestOutput/unzipTool
    ${result} =    Run Process  LD_LIBRARY_PATH\=/opt/sophos-spl/base/lib64/ ${unzipTool} /tmp/upload.zip /tmp/unpackzip/ password  shell=True
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0   "zip utility failed: Reason ${result.stderr}"
    File Should exist  /tmp/unpackzip/file
    File Should Contain  /tmp/unpackzip/file     tempfilecontent