*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot

Library    ${LIBS_DIRECTORY}/OnFail.py

Suite Setup     RA Telemetry Suite Setup
Suite Teardown  Require Uninstalled

Test Setup  RA Telemetry Test Setup
Test Teardown  RA Telemetry Test Teardown

Force Tags  LOAD7
Default Tags   RESPONSE_ACTIONS_PLUGIN  TAP_TESTS

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
    Install Response Actions Directly
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}


#Run Command Action
Telemetry Reported For Run Command Action Expired
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction_expired.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${RUN_COMMAND_COUNT}    ${RUN_COMMAND_FAILED_COUNT}    ${RUN_COMMAND_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Run Command Action Timeout Exceeded
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction4.json
    Wait Until Created    ${RESPONSE_JSON}

    wait_for_log_contains_from_mark  ${response_mark}  Response Actions plugin sending failed response to Central on behalf of Action Runner process


    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${RUN_COMMAND_COUNT}    ${RUN_COMMAND_FAILED_COUNT}    ${RUN_COMMAND_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Run Command Action Failure Not Timeout Or Expiry
    # Exclude on SLES until LINUXDAR-7306 is fixed
    [Tags]  EXCLUDE_SLES12
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction_failure.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${RUN_COMMAND_COUNT}    ${RUN_COMMAND_FAILED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


#Upload File Action
Telemetry Reported For Upload File Action Expired
    # Exclude on SLES until LINUXDAR-7306 is fixed
    [Tags]  EXCLUDE_SLES12
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/UploadFileAction_expired.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FILE_COUNT}    ${UPLOAD_FILE_FAILED_COUNT}    ${UPLOAD_FILE_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload File Action Timeout Exceeded
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/UploadFileAction_timeout.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FILE_COUNT}    ${UPLOAD_FILE_FAILED_COUNT}    ${UPLOAD_FILE_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload File Action Failure Not Timeout Or Expiry
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/UploadFileAction_failure.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FILE_COUNT}    ${UPLOAD_FILE_FAILED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


#Upload Folder Action
Telemetry Reported For Upload Folder Action Expired
    Create Directory    /tmp/test_folder
    Register Cleanup    Remove Directory    /tmp/test_folder

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/UploadFolderAction_expired.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FOLDER_COUNT}    ${UPLOAD_FOLDER_FAILED_COUNT}    ${UPLOAD_FOLDER_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload Folder Action Timeout Exceeded
    Create Directory    /tmp/test_folder
    Register Cleanup    Remove Directory    /tmp/test_folder

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/UploadFolderAction_timeout.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FOLDER_COUNT}    ${UPLOAD_FOLDER_FAILED_COUNT}    ${UPLOAD_FOLDER_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Upload Folder Action Failure Not Timeout Or Expiry
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/UploadFolderAction_failure.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${UPLOAD_FOLDER_COUNT}    ${UPLOAD_FOLDER_FAILED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


#Download File Action
Telemetry Reported For Download File Action Expired
    Simulate Download Action Now    ${SUPPORT_FILES}/CentralXml/DownloadFileAction_expired.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${DOWNLOAD_FILE_COUNT}    ${DOWNLOAD_FILE_FAILED_COUNT}    ${DOWNLOAD_FILE_EXPIRED_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Download File Action Timeout Exceeded
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/DownloadFileAction_timeout.json
    Wait Until Created    ${RESPONSE_JSON}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}

    @{non_zero_telemetry_fields} =  Create List   ${DOWNLOAD_FILE_COUNT}    ${DOWNLOAD_FILE_FAILED_COUNT}    ${DOWNLOAD_FILE_TIMEOUT_COUNT}
    Check RA Telemetry Json Is Correct  ${telemetryFileContents}    ${non_zero_telemetry_fields}


Telemetry Reported For Download File Action Failure Not Timeout Or Expiry
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/DownloadFileAction_failure.json
    Wait Until Created    ${RESPONSE_JSON}

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
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove File  ${EXE_CONFIG_FILE}
