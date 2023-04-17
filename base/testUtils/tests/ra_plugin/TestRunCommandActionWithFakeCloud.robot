*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OnFail.py
Library    ${LIBS_DIRECTORY}/CentralUtils.py
Suite Setup     RA Run Command Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup         RA Run Command With Fake Cloud Test Setup
Test Teardown      RA Run Command Test Teardown

Force Tags  LOAD5
Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Variables ***
${RESPONSE_JSON}        ${MCS_DIR}/response/CORE_id1_response.json

*** Test Cases ***
Test Run Command Action End To End With Fake Cloud
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    @{commands_list} =  Create List   echo -n one  sleep 1  echo -n two > /tmp/test.txt

    send_run_command_action_from_fake_cloud  ${commands_list}
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent run command response for ID

    Wait Until Keyword Succeeds
    ...    1 secs
    ...    10 secs
    ...    Check Cloud Server Log Contains   {"duration":0,"exitCode":0,"stdErr":"","stdOut":"one"}
    check cloud server log contains pattern   {"duration":[12],"exitCode":0,"stdErr":"","stdOut":""}
    Check Cloud Server Log Contains   {"duration":0,"exitCode":0,"stdErr":"","stdOut":""}
    Check Cloud Server Log Contains   sophos.mgt.response.RunCommands

    File Should exist  /tmp/test.txt
    ${test_contents} =  Get File    /tmp/test.txt
    Should Be Equal As Strings  ${test_contents}    two

Test Run Command Action With 400 response
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    ${mcsrouter_mark} =  get_mark_for_mcsrouter_log
    ${cloud_server_mark} =  mark_cloud_server_log

    @{commands_list} =  Create List   echo -n one

    send_run_command_action_from_fake_cloud  ${commands_list}  return400=${True}
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   ${25}
    wait_for_log_contains_from_mark  ${action_mark}    Sent run command response for ID

    wait_for_log_contains_from_mark  ${cloud_server_mark}  {"duration":0,"exitCode":0,"stdErr":"","stdOut":"one"}  ${10}
    wait_for_log_contains_from_mark  ${cloud_server_mark}  sophos.mgt.response.RunCommands

    Wait For Log Contains From Mark    ${mcsrouter_mark}    Failed to send response (CORE : corrid) : MCS HTTP Error: code=400
    Wait For Log Contains From Mark    ${mcsrouter_mark}    Discarding response 'corrid' due to rejection with HTTP 400 error from Sophos Central

    dump_from_mark  ${mcsrouter_mark}


*** Keywords ***

RA Run Command With Fake Cloud Test Setup
    Override LogConf File as Global Level  DEBUG

RA Run Command Test Teardown
    Start Response Actions
    Run Keyword If Test Failed    Dump Cloud Server Log
    General Test Teardown
    Remove File    /tmp/test.txt
    Run Keyword If Test Failed    Log File    ${RESPONSE_JSON}
    Remove File    ${RESPONSE_JSON}
