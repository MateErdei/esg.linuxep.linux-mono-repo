*** Settings ***
Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  ../mcs_router/McsRouterResources.robot

Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OnFail.py
Library    ${LIBS_DIRECTORY}/CentralUtils.py

Suite Setup     RA Command Direct Suite setup
Suite Teardown  RA Suite Teardown

Test Setup         RA Run Command Test Setup
Test Teardown      RA Run Command Test Teardown

Force Tags  LOAD5
Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Variables ***
${RESPONSE_JSON}        ${MCS_DIR}/response/CORE_id1_response.json

*** Test Cases ***

Test Run Command Action and Verify Response JSON on Success
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created  /tmp/test.txt
    Wait Until Created    ${RESPONSE_JSON}

    &{cmd_output1} =	Create Dictionary	stdOut=hello    stdErr=    exitCode=${0}
    &{cmd_output2} =	Create Dictionary	stdOut=    stdErr=    exitCode=${0}
    @{cmd_output_list} =  Create List   ${cmd_output1}    ${cmd_output2}
    verify_run_command_response    ${RESPONSE_JSON}   ${0}    ${cmd_output_list}


Test Run Command Action and Verify Response JSON with Ignore Error False
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction2.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    &{cmd_output1} =	Create Dictionary	stdOut=hello    stdErr=    exitCode=${0}
    &{cmd_output2} =	Create Dictionary	stdOut=    stdErr=    exitCode=${1}
    @{cmd_output_list} =  Create List   ${cmd_output1}    ${cmd_output2}
    verify_run_command_response    ${RESPONSE_JSON}   ${1}    ${cmd_output_list}


Test Run Command Action and Verify Response JSON with Ignore Error True
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction3.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    &{cmd_output1} =	Create Dictionary	stdOut=hello    stdErr=    exitCode=${0}
    &{cmd_output2} =	Create Dictionary	stdOut=    stdErr=    exitCode=${1}
    &{cmd_output3} =	Create Dictionary	stdOut=I will run    stdErr=    exitCode=${0}
    @{cmd_output_list} =  Create List   ${cmd_output1}    ${cmd_output2}    ${cmd_output3}
    verify_run_command_response    ${RESPONSE_JSON}   ${1}    ${cmd_output_list}

Test Run Command Action and Verify Response JSON when Timeout Exceeded
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction4.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    # 2 is timeout
    verify_run_command_response    ${RESPONSE_JSON}   ${2}    expect_timeout=${True}

Test Run Command Action and Verify Commands Ran In Correct Order
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction5.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    &{cmd_output1} =	Create Dictionary	stdOut=command 1    stdErr=    exitCode=${0}
    &{cmd_output2} =	Create Dictionary	stdOut=command 2    stdErr=    exitCode=${0}
    &{cmd_output3} =	Create Dictionary	stdOut=command 3    stdErr=    exitCode=${0}
    &{cmd_output4} =	Create Dictionary	stdOut=command 4    stdErr=    exitCode=${0}
    @{cmd_output_list} =  Create List   ${cmd_output1}    ${cmd_output2}    ${cmd_output3}    ${cmd_output4}
    verify_run_command_response    ${RESPONSE_JSON}   ${0}    ${cmd_output_list}

    Check Log Contains In Order   ${ACTIONS_RUNNER_LOG_PATH}
    ...    Running 4 commands
    ...    Running command 1
    ...    Command 1 exit code: 0
    ...    Running command 2
    ...    Command 2 exit code: 0
    ...    Running command 3
    ...    Command 3 exit code: 0
    ...    Running command 4
    ...    Command 4 exit code: 0
    ...    Overall command result: 0

Test Run Command Action Handles Large Stdout
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction6.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    ${output} =     Evaluate    "".join(["a"] * 10000000)
    &{cmd_output1} =	Create Dictionary	stdOut=${output}    stdErr=    exitCode=${0}
    @{cmd_output_list} =  Create List   ${cmd_output1}
    verify_run_command_response    ${RESPONSE_JSON}   ${0}    ${cmd_output_list}

    Check Log Contains In Order   ${ACTIONS_RUNNER_LOG_PATH}
    ...    Running 1 command
    ...    Running command 1
    ...    Command 1 exit code: 0
    ...    Overall command result: 0

Test Run Command Action Handles Malformed Json Request
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction_malformed.json
    wait_for_log_contains_from_mark    ${response_mark}    Cannot parse action with error

Test Run Command Action Handles Missing Binary
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction7.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    # bash will print different output on different platforms for missing binaries, so get the expected stderr
    ${result} =  Run Process    /bin/bash    -c    commandThatDoesNotExist

    &{cmd_output} =	Create Dictionary	stdOut=    stdErr=${result.stderr}\n    exitCode=${127}
    @{cmd_output_list} =  Create List   ${cmd_output}
    verify_run_command_response    ${RESPONSE_JSON}   ${1}    ${cmd_output_list}

    Check Log Contains In Order   ${ACTIONS_RUNNER_LOG_PATH}
    ...    Running 1 command
    ...    Running command 1
    ...    Command 1 exit code: 127
    ...    Overall command result: 1

Test Run Command Action Handles Expired Action
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction_expired.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    wait_for_log_contains_from_mark  ${action_mark}  Command id1 has expired so will not be run
    Wait Until Created    ${RESPONSE_JSON}
    ${response_content}=  Get File    ${RESPONSE_JSON}
    Should Be Equal As Strings   ${response_content}    {"commandResults":[],"duration":0,"result":4,"startedAt":0,"type":"sophos.mgt.response.RunCommands"}


Test Run Command Action Does Not Block RA Plugin Stopping
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Simulate Run Command Action Now    ${SUPPORT_FILES}/CentralXml/RunCommandAction_sleep.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Keyword Succeeds
    ...  3s
    ...  1s
    ...  Check sleep Running

    ${time_before} =  Get Current Time
    Stop Response Actions
    ${time_after} =  Get Current Time
    ${took_less_than_5_seconds}    Evaluate    (${time_after}-${time_before}) < 5
    Should Be True    ${took_less_than_5_seconds}

    Wait Until Created    ${RESPONSE_JSON}
    ${response_content}=  Get File    ${RESPONSE_JSON}
    Should Be Equal As Strings   ${response_content}    {"result":3,"type":"sophos.mgt.response.RunCommands"}

*** Keywords ***

RA Command Direct Suite setup
    Run Full Installer
    Install Response Actions Directly

RA Run Command Test Setup
    # Stop MCS Router so that it does not consume the response files we want to validate.
    Stop MCSRouter


RA Run Command Test Teardown
    Start Response Actions
    Run Keyword If Test Failed    Dump Cloud Server Log
    General Test Teardown
    Remove File    /tmp/test.txt
    Run Keyword If Test Failed    Log File    ${RESPONSE_JSON}
    Remove File    ${RESPONSE_JSON}

Simulate Run Command Action Now
    [Arguments]  ${action_json_file}=${SUPPORT_FILES}/CentralXml/RunCommandAction.json    ${id_suffix}=id1
    Copy File   ${action_json_file}  ${SOPHOS_INSTALL}/tmp/RunCommandAction.json
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${SOPHOS_INSTALL}/tmp/RunCommandAction.json   shell=True
    Should Be Equal As Integers    ${result.rc}    0  Failed to replace permission to file. Reason: ${result.stderr}
    Move File   ${SOPHOS_INSTALL}/tmp/RunCommandAction.json  ${SOPHOS_INSTALL}/base/mcs/action/CORE_${id_suffix}_request_2030-02-27T13:45:35.699544Z_144444000000004.json

Stop MCSRouter
    ${result} =  Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Run Keyword If  ${result.rc} != 0  log  ${result.stdout}
    Run Keyword If  ${result.rc} != 0  log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0

Check sleep Running
    ${result} =    Run Process  pgrep    sleep
    Should Be Equal As Integers    ${result.rc}    0