*** Settings ***
Library    ${COMMON_TEST_LIBS}/OnFail.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Suite Setup     RA Telemetry Suite Setup
Suite Teardown  Require Uninstalled

Test Setup  RA Telemetry Test Setup
Test Teardown  RA Telemetry Test Teardown

Force Tags  RESPONSE_ACTIONS_PLUGIN  TAP_PARALLEL2

*** Variables ***
#Telemetry Fields
${RUN_COMMAND_COUNT}    run-command-actions
${RUN_COMMAND_FAILED_COUNT}    run-command-failed
${RUN_COMMAND_TIMEOUT_COUNT}    run-command-timeout-actions
${RUN_COMMAND_EXPIRED_COUNT}    run-command-expired-actions
${UPLOAD_FILE_COUNT}    upload-file-count
${UPLOAD_FILE_FAILED_COUNT}    upload-file-overall-failures
${UPLOAD_FILE_TIMEOUT_COUNT}    upload-file-timeout-failures
${UPLOAD_FILE_EXPIRED_COUNT}    upload-file-expiry-failures
${UPLOAD_FOLDER_COUNT}    upload-folder-count
${UPLOAD_FOLDER_FAILED_COUNT}    upload-folder-overall-failures
${UPLOAD_FOLDER_TIMEOUT_COUNT}    upload-folder-timeout-failures
${UPLOAD_FOLDER_EXPIRED_COUNT}    upload-folder-expiry-failures
${DOWNLOAD_FILE_COUNT}    download-file-count
${DOWNLOAD_FILE_FAILED_COUNT}    download-file-overall-failures
${DOWNLOAD_FILE_TIMEOUT_COUNT}    download-file-timeout-failures
${DOWNLOAD_FILE_EXPIRED_COUNT}    download-file-expiry-failures


*** Test Cases ***
RA Plugin Reports Telemetry Correctly
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}


#Run Command Action
Telemetry Reported For Run Command Action Expired
    ${id} =  Set Variable  id1
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction_expired.json    ${id}
    Wait Until Created    ${RESPONSE_JSON}

    wait_for_log_contains_from_mark  ${response_mark}  Failed action ${id} with exit code 4
    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${RUN_COMMAND_COUNT}    ${RUN_COMMAND_FAILED_COUNT}    ${RUN_COMMAND_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Run Command Action Timeout Exceeded
    ${id} =  Set Variable  id2
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction4.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${RUN_COMMAND_COUNT}    ${RUN_COMMAND_FAILED_COUNT}    ${RUN_COMMAND_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Run Command Action Failure Not Timeout Or Expiry
    ${id} =  Set Variable  id3
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction_failure.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${RUN_COMMAND_COUNT}    ${RUN_COMMAND_FAILED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


#Upload File Action
Telemetry Reported For Upload File Action Expired
    ${id} =  Set Variable  id4
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/UploadFileAction_expired.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Failed action ${id} with exit code 4
    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FILE_COUNT}    ${UPLOAD_FILE_FAILED_COUNT}    ${UPLOAD_FILE_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload File Action Timeout Exceeded
    ${id} =  Set Variable  id5
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    generate_file  /tmp/largefile  ${99999}
    Register Cleanup  Remove File  /tmp/largefile
    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/UploadFileAction_timeout.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FILE_COUNT}    ${UPLOAD_FILE_FAILED_COUNT}    ${UPLOAD_FILE_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload File Action Failure Not Timeout Or Expiry
    ${id} =  Set Variable  id6
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action   ${SUPPORT_FILES}/CentralXml/UploadFileAction_failure.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FILE_COUNT}    ${UPLOAD_FILE_FAILED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


#Upload Folder Action
Telemetry Reported For Upload Folder Action Expired
    ${id} =  Set Variable  id7
    Create Directory    /tmp/test_folder
    Register Cleanup    Remove Directory    /tmp/test_folder

    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action   ${SUPPORT_FILES}/CentralXml/UploadFolderAction_expired.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Failed action ${id} with exit code 4
    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FOLDER_COUNT}    ${UPLOAD_FOLDER_FAILED_COUNT}    ${UPLOAD_FOLDER_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload Folder Action Timeout Exceeded
    ${id} =  Set Variable  id8

    Create Directory    /tmp/test_folder
    Register Cleanup    Remove Directory    /tmp/test_folder

    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/UploadFolderAction_timeout.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FOLDER_COUNT}    ${UPLOAD_FOLDER_FAILED_COUNT}    ${UPLOAD_FOLDER_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload Folder Action Failure Not Timeout Or Expiry
    ${id} =  Set Variable  id9
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/UploadFolderAction_failure.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FOLDER_COUNT}    ${UPLOAD_FOLDER_FAILED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


#Download File Action
Telemetry Reported For Download File Action Expired
    ${id} =  Set Variable  id10
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/DownloadFileAction_expired.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Failed action ${id} with exit code 4
    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${DOWNLOAD_FILE_COUNT}    ${DOWNLOAD_FILE_FAILED_COUNT}    ${DOWNLOAD_FILE_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Download File Action Timeout Exceeded
    ${id} =  Set Variable  id11
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/DownloadFileAction_timeout.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${DOWNLOAD_FILE_COUNT}    ${DOWNLOAD_FILE_FAILED_COUNT}    ${DOWNLOAD_FILE_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Download File Action Failure Not Timeout Or Expiry
    ${id} =  Set Variable  id12
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/DownloadFileAction_failure.json    ${id}
    Wait Until Created    ${RESPONSE_JSON_PATH}CORE_${id}_response.json

    wait_for_log_contains_from_mark  ${response_mark}  Finished action: ${id}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${DOWNLOAD_FILE_COUNT}    ${DOWNLOAD_FILE_FAILED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


*** Keywords ***
RA Telemetry Suite Setup
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Copy Telemetry Config File in To Place
    Create File    ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [telemetry]\nVERBOSITY=DEBUG\n

RA Telemetry Test Setup
    Install Response Actions Directly
    Prepare To Run Telemetry Executable

RA Telemetry Test Teardown
    General Test Teardown
    Uninstall Response Actions
    Run Keyword And Ignore Error   Remove File   ${SOPHOS_INSTALL}/base/telemetry/cache/ra-telemetry.json
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server And Remove Telemetry Output
    Remove File  ${EXE_CONFIG_FILE}
