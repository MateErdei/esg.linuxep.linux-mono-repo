*** Settings ***
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/OnFail.py
Library    ${COMMON_TEST_LIBS}/CentralUtils.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Suite Setup     RA Run Command Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup         RA Run Command With Fake Cloud Test Setup
Test Teardown      RA Run Command Test Teardown

Force Tags   RESPONSE_ACTIONS_PLUGIN  TAP_PARALLEL2

*** Test Cases ***
Test Run Command Action End To End With Fake Cloud
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    @{commands_list} =  Create List   echo -n one  sleep 1  echo -n two > /tmp/test.txt

    ${server_log_path} =  cloud_server_log_path
    ${server_mark} =  mark_log_size  ${server_log_path}

    send_run_command_action_from_fake_cloud  ${commands_list}
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent run command response for ID

    wait for cloud server log contains pattern  .\*{"duration":[0-9]+,"exitCode":0,"stdErr":"","stdOut":"one"}
    ...  mark=${server_mark}
    ...  timeout=${10}

    wait for cloud server log contains pattern   .\*{"duration":[0-9]+,"exitCode":0,"stdErr":"","stdOut":""}  mark=${server_mark}
    wait for cloud server log contains pattern   .\*{"duration":[0-9]+,"exitCode":0,"stdErr":"","stdOut":""}  mark=${server_mark}
    Check Cloud Server Log Contains   sophos.mgt.response.RunCommands

    File Should exist  /tmp/test.txt
    ${test_contents} =  Get File    /tmp/test.txt
    Should Be Equal As Strings  ${test_contents}    two


Test Run Command Action End To End With Fake Cloud Lots Of Serial Actions
    ${iterations} =    Set Variable    2000
    ${dest_dir} =    Set Variable    /tmp/testdir/
    Create Directory    ${dest_dir}
    Register Cleanup    Remove Directory     ${dest_dir}    true

    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    @{commands_list} =  Create List
    FOR    ${item}     IN RANGE     0    ${iterations}
        Append To List    ${commands_list}    echo -n ${item} > /tmp/testdir/file${item}.txt
    END

    ${server_log_path} =  cloud_server_log_path
    ${server_mark} =  mark_log_size  ${server_log_path}

    send_run_command_action_from_fake_cloud  ${commands_list}
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Sent run command response for ID

    ${dir_contents} =    Count Files In Directory    ${dest_dir}
    Should Be Equal As Integers  ${iterations}    ${dir_contents}


Test Run Command Action With 400 response
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    ${mcsrouter_mark} =  get_mark_for_mcsrouter_log
    ${cloud_server_mark} =  mark_cloud_server_log

    @{commands_list} =  Create List   echo -n one

    send_run_command_action_from_fake_cloud  ${commands_list}  return400=${True}
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   ${25}
    wait_for_log_contains_from_mark  ${action_mark}    Sent run command response for ID

    wait for cloud server log contains pattern  .\*{"duration":[0-9]+,"exitCode":0,"stdErr":"","stdOut":"one"}
    ...  mark=${cloud_server_mark}
    ...  timeout=${10}
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
