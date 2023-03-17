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
    ...  Check Cloud Server Log Contains    \"fileName\":\"file\",\"httpStatus\":200,\"result\":0,\"sha256\":\"382a1f7b76ca05728a4e9e6b3a4877f8634fb33b08decc5251933705a684f34f\"

