*** Settings ***
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/OnFail.py
Library    ${COMMON_TEST_LIBS}/CentralUtils.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Suite Setup     RA Action With Register Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup  RA Test Setup
Test Teardown  RA Test Teardown

Force Tags   RESPONSE_ACTIONS_PLUGIN  TAP_PARALLEL1

*** Variables ***
#if these change, change in Create_And_Send_Download_File_From_Fake_Cloud also
${RESPONSE_ACTIONS_TMP_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/tmp/
${MOUNT_DIR}   ${TESTDIR}/mount
${DOWNLOAD_TARGET_PATH}    /tmp/folder/
${DOWNLOAD_FILENAME_ZIP}    download.zip
${DOWNLOAD_FILENAME_TXT}    download.txt
${TMP_DOWNLOAD_ZIP}    tmp_download.zip

*** Test Cases ***
RA Plugin downloads with decompress set to false successfully
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}

    Create_And_Send_Download_File_From_Fake_Cloud

    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP}

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":0


RA Plugin downloads and extracts a single file successfully
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory  /tmp/folder  recursive=${TRUE}

    Create_And_Send_Download_File_From_Fake_Cloud   ${TRUE}  ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_TXT}

    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}tmp/${DOWNLOAD_FILENAME_TXT} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   ${DOWNLOAD_TARGET_PATH}tmp/${DOWNLOAD_FILENAME_TXT}
    File Should Not Exist    ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP}

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}extract/${DOWNLOAD_FILENAME_TXT}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"httpStatus\":200,\"result\":0

RA Plugin handles decompression of non zip file appropriately
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory  /tmp/folder  recursive=${TRUE}

    Create And Send Download File From Fake Cloud NotZip

    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   invalid zip file

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"errorType\":\"invalid_archive\",\"httpStatus\":200,\"result\":1


RA Plugin handles download to mounts appropriately
    Require Filesystem    ext4
    ${image} =    Copy And Extract Image     ext4FileSystem
    Mount Image    ${MOUNT_DIR}      ${image}      ext4
    Register Cleanup  Remove Directory  ${TESTDIR}  recursive=True
    Register Cleanup  Unmount Image Internal  ${MOUNT_DIR}

    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Create And Send Download File From Fake Cloud    targetPath=${MOUNT_DIR}/${DOWNLOAD_FILENAME_ZIP}

    Wait for log contains from mark  ${response_mark}    Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}    ${MOUNT_DIR}/${DOWNLOAD_FILENAME_ZIP} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log

    File Should Exist    ${MOUNT_DIR}/${DOWNLOAD_FILENAME_ZIP}
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":0


RA Plugin handles read only mount appropriately
    Require Filesystem    ext4
    ${image} =    Copy And Extract Image     ext4FileSystem
    Mount Image Read Only    ${MOUNT_DIR}      ${image}      ext4
    Register Cleanup  Remove Directory  ${TESTDIR}  recursive=True
    Register Cleanup  Unmount Image Internal  ${MOUNT_DIR}

    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Create And Send Download File From Fake Cloud    targetPath=${MOUNT_DIR}/${DOWNLOAD_FILENAME_ZIP}

    Wait for log contains from mark  ${response_mark}  Failed action correlation-id with exit code 1   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}    Failed to copy file: '${RESPONSE_ACTIONS_TMP_PATH}${TMP_DOWNLOAD_ZIP}' to '${MOUNT_DIR}/${DOWNLOAD_FILENAME_ZIP}'

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"errorType\":\"access_denied\",\"httpStatus\":200,\"result\":1

RA Plugin handles download of single file to mount with not enough space appropriately
    Require Filesystem    ext4
    ${image} =    Copy And Extract Image     ext4FileSystem
    Mount Image    ${MOUNT_DIR}      ${image}      ext4
    Register Cleanup  Remove Directory  ${TESTDIR}  recursive=True
    Register Cleanup  Unmount Image Internal  ${MOUNT_DIR}

    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Create And Send Download File From Fake Cloud    ${TRUE}    targetPath=${MOUNT_DIR}/${DOWNLOAD_FILENAME_ZIP}    specifySize=20000000

    Wait for log contains from mark  ${response_mark}    Failed action correlation-id with exit code    25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15

    Wait for log contains from mark  ${action_mark}  Failed to copy file: '${RESPONSE_ACTIONS_TMP_PATH}extract/tmp/${DOWNLOAD_FILENAME_TXT}' to '${MOUNT_DIR}/tmp/${DOWNLOAD_FILENAME_TXT}', failed to complete writing to file, check space available on device.


    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log

    File Should Not Exist    ${MOUNT_DIR}/tmp/${DOWNLOAD_FILENAME_TXT}
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}extract/${DOWNLOAD_FILENAME_TXT}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":1


RA Plugin handles download of multiple files to mount with not enough space appropriately
    Require Filesystem    ext4
    ${image} =    Copy And Extract Image     ext4FileSystem
    Mount Image    ${MOUNT_DIR}      ${image}      ext4
    Register Cleanup  Remove Directory  ${TESTDIR}  recursive=True
    Register Cleanup  Unmount Image Internal  ${MOUNT_DIR}

    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Create And Send Download File From Fake Cloud    ${TRUE}    targetPath=${MOUNT_DIR}/${DOWNLOAD_FILENAME_ZIP}    specifySize=20000000    multipleFiles=${TRUE}

    Wait for log contains from mark  ${response_mark}    Failed action correlation-id with exit code    25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}    failed to complete writing to file, check space available on device.


    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log

    FOR    ${item}    IN    0    1    2    3    4    5    6    7    8    9
        File Should Not Exist     ${MOUNT_DIR}/tmp/${DOWNLOAD_FILENAME_TXT}${item}
        check_log_contains_after_mark    ${ACTIONS_RUNNER_LOG_PATH}    Removing file ${MOUNT_DIR}/tmp/${DOWNLOAD_FILENAME_TXT}${item} as move file failed    ${action_mark}
    END

    File Should Not Exist    ${MOUNT_DIR}/tmp/${DOWNLOAD_FILENAME_TXT}
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}
    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}extract/${DOWNLOAD_FILENAME_TXT}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":1



RA Plugin downloads and extracts multiple files successfully
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}

    Create And Send Download File From Fake Cloud    ${TRUE}  ${DOWNLOAD_TARGET_PATH}   multipleFiles=${TRUE}

    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   ${DOWNLOAD_TARGET_PATH}tmp/${DOWNLOAD_FILENAME_TXT}

    FOR    ${item}    IN    0    1    2    3    4    5    6    7    8    9
        File Should Exist     ${DOWNLOAD_TARGET_PATH}tmp/${DOWNLOAD_FILENAME_TXT}${item}
    END

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":0


RA Plugin downloads and extracts multiple files and multiple subdirectories successfully
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}

    Create And Send Download File From Fake Cloud    ${TRUE}  ${DOWNLOAD_TARGET_PATH}   multipleFiles=${TRUE}    subDirectories=${TRUE}

    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   ${DOWNLOAD_TARGET_PATH}/tmp/subDir1/subDir2/${DOWNLOAD_FILENAME_TXT}

    FOR    ${item}    IN    0    1    2    3    4    5    6    7    8    9
        File Should Exist     ${DOWNLOAD_TARGET_PATH}/tmp/${item}/${DOWNLOAD_FILENAME_TXT}${item}
    END

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":0

RA Plugin cleans up before starting download action
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}

    Create File     ${RESPONSE_ACTIONS_TMP_PATH}${TMP_DOWNLOAD_ZIP}

    Create And Send Download File From Fake Cloud

    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP} downloaded successfully

    Check Log Contains  Received HTTP GET Request  ${HTTPS_LOG_FILE_PATH}  https server log
    File Should Exist   ${DOWNLOAD_TARGET_PATH}${DOWNLOAD_FILENAME_ZIP}

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains   \"httpStatus\":200,\"result\":0


RA Plugin fails a real url due to authorization not file to long
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    /tmp/folder/  recursive=${TRUE}

    Create And Send Download File From Fake Cloud RealURL

    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   Failed to download, Error code: 403

    File Should Not Exist    ${RESPONSE_ACTIONS_TMP_PATH}${DOWNLOAD_FILENAME_ZIP}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    \"errorType\":\"network_error\",\"httpStatus\":403,\"result\":1

RA Plugin downloads a file successfully with password protected decompression
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}
    
    ${zip_password} =    Set Variable    password
    ${zip_file} =    Set Variable    /tmp/testdownload.zip
    ${zipped_file} =    Set Variable    /tmp/test.txt
    ${zipped_file_contents} =    Set Variable    test contents
    Create File     ${zipped_file}    ${zipped_file_contents}
    Register Cleanup  Remove File    ${zipped_file}
    Run Process    zip  -r    ${zip_file}    ${zipped_file}    -P    ${zip_password}
    Register Cleanup  Remove File    ${zip_file}
   
    Send_Download_File_From_Fake_Cloud   ${zip_file}    ${TRUE}    ${DOWNLOAD_TARGET_PATH}    ${zip_password}
    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}tmp/test.txt downloaded successfully

    File Should Contain      ${DOWNLOAD_TARGET_PATH}tmp/test.txt    ${zipped_file_contents}

RA Plugin downloads and extracts BZIP2 files successfully
    # LINUXDAR-8640 - Disabled until minizip built with BZIP2 capabilities enabled
    [Tags]    TESTFAILURE
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}
    
    ${zip_password} =    Set Variable    password
    ${zip_file} =    Set Variable    /tmp/testdownload.zip
    ${zipped_file} =    Set Variable    /tmp/test.txt
    ${zipped_file_contents} =    Set Variable    test contents
    Create File     ${zipped_file}    ${zipped_file_contents}
    Register Cleanup  Remove File    ${zipped_file}
    Run    bzip2 -c ${zipped_file} > ${zip_file}
    Register Cleanup  Remove File    ${zip_file}
   
    Send_Download_File_From_Fake_Cloud   ${zip_file}    ${TRUE}    ${DOWNLOAD_TARGET_PATH}    ${zip_password}
    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}tmp/test.txt downloaded successfully

    File Should Contain      ${DOWNLOAD_TARGET_PATH}tmp/test.txt    ${zipped_file_contents}

RA Plugin downloads and extracts ZST files successfully
    # LINUXDAR-8640 - Disabled until minizip built with ZST capabilities enabled
    [Tags]    TESTFAILURE
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Register Cleanup  Remove Directory    ${DOWNLOAD_TARGET_PATH}  recursive=${TRUE}
    
    ${zip_password} =    Set Variable    password
    ${zip_file} =    Set Variable    /tmp/testdownload.zip
    ${zipped_file} =    Set Variable    /tmp/test.txt
    ${zipped_file_contents} =    Set Variable    test contents
    Create File     ${zipped_file}    ${zipped_file_contents}
    Register Cleanup  Remove File    ${zipped_file}
    Run    zstd ${zipped_file} -o ${zip_file}
    Register Cleanup  Remove File    ${zip_file}
   
    Send_Download_File_From_Fake_Cloud   ${zip_file}    ${TRUE}    ${DOWNLOAD_TARGET_PATH}    ${zip_password}
    Wait for log contains from mark  ${response_mark}  Action correlation-id has succeeded   25
    Wait for log contains from mark  ${action_mark}  Sent download file response for ID correlation-id to Central   15
    Wait for log contains from mark  ${action_mark}   ${DOWNLOAD_TARGET_PATH}tmp/test.txt downloaded successfully

    File Should Contain      ${DOWNLOAD_TARGET_PATH}tmp/test.txt    ${zipped_file_contents}