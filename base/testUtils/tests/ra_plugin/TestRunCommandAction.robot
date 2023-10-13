*** Settings ***
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OnFail.py
Library    ${LIBS_DIRECTORY}/CentralUtils.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Suite Setup     RA Command Direct Suite setup
Suite Teardown  RA Suite Teardown

Test Setup         RA Run Command Test Setup
Test Teardown      RA Run Command Test Teardown

Force Tags  RESPONSE_ACTIONS_PLUGIN    TAP_PARALLEL4

*** Test Cases ***

Test Run Command Action and Verify Response JSON on Success
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created  /tmp/test.txt
    Wait Until Created    ${RESPONSE_JSON}

    &{cmd_output1} =	Create Dictionary	stdOut=hello    stdErr=    exitCode=${0}
    &{cmd_output2} =	Create Dictionary	stdOut=    stdErr=    exitCode=${0}
    @{cmd_output_list} =  Create List   ${cmd_output1}    ${cmd_output2}
    verify_run_command_response    ${RESPONSE_JSON}   ${0}    ${cmd_output_list}


Test Run Command Action and Verify Response JSON with Ignore Error False
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction2.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    &{cmd_output1} =	Create Dictionary	stdOut=hello    stdErr=    exitCode=${0}
    &{cmd_output2} =	Create Dictionary	stdOut=    stdErr=    exitCode=${1}
    @{cmd_output_list} =  Create List   ${cmd_output1}    ${cmd_output2}
    verify_run_command_response    ${RESPONSE_JSON}   ${1}    ${cmd_output_list}


Test Run Command Action and Verify Response JSON with Ignore Error True
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction3.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    &{cmd_output1} =	Create Dictionary	stdOut=hello    stdErr=    exitCode=${0}
    &{cmd_output2} =	Create Dictionary	stdOut=    stdErr=    exitCode=${1}
    &{cmd_output3} =	Create Dictionary	stdOut=I will run    stdErr=    exitCode=${0}
    @{cmd_output_list} =  Create List   ${cmd_output1}    ${cmd_output2}    ${cmd_output3}
    verify_run_command_response    ${RESPONSE_JSON}   ${1}    ${cmd_output_list}

Test Run Command Action and Verify Response JSON when Timeout Exceeded
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction4.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    Wait Until Created    ${RESPONSE_JSON}

    # 2 is timeout
    verify_run_command_response    ${RESPONSE_JSON}   ${2}    expect_timeout=${True}


Test Run Command Action and Verify Commands Ran In Correct Order
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction5.json
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

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction6.json
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

Test Run Command Action Handles Missing Binary
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction7.json
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

    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction_expired.json
    wait_for_log_contains_from_mark  ${action_mark}  Performing run command action:
    wait_for_log_contains_from_mark  ${action_mark}  Command id1 has expired so will not be run
    Wait Until Created    ${RESPONSE_JSON}
    ${response_content}=  Get File    ${RESPONSE_JSON}
    Should Be Equal As Strings   ${response_content}    {"errorMessage":"Command id1 has expired so will not be run.","result":4,"type":"sophos.mgt.response.RunCommands"}


Test Run Command Action Does Not Block RA Plugin Stopping
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction_sleep.json
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

    &{cmd_output1} =	Create Dictionary	stdOut=    stdErr=    exitCode=${15}
    @{cmd_output_list} =  Create List   ${cmd_output1}
    verify_run_command_response    ${RESPONSE_JSON}   ${1}    ${cmd_output_list}

Test Run Command Action Process Traps SIGTERM And Gets Killed In A Specified Amount Of Time
    [Tags]    EXCLUDE_ON_COVERAGE
    Override LogConf File as Global Level  DEBUG

    ${allowed_leeway} =    Set variable    0.5
    ${iterations_passed} =    Set variable    0
    ${num_iterations} =    Set variable    10
    ${pass_percentage_threshold} =    Set variable    0.7

    FOR    ${iteration}    IN RANGE    0    ${num_iterations}
        ${time_taken} =    Simulate Response Action And Return Time Taken To Kill Process
        ${time_taken_check} =   Evaluate    ${time_taken} <= (2 + ${allowed_leeway})
        IF    ${time_taken_check} is ${TRUE}
            ${iterations_passed} =    Evaluate    ${iterations_passed} + 1
        END
    END

    ${pass_percentage} =    Evaluate    (${iterations_passed} / ${num_iterations}) >= ${pass_percentage_threshold}
    Should Be True    ${pass_percentage}

*** Keywords ***
Simulate Response Action And Return Time Taken To Kill Process
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Simulate Response Action    ${SUPPORT_FILES}/CentralXml/RunCommandAction_trapSIGTERM.json
    # Note: the timeout value in the command json file IS NOT the amount of time given for the command process to stop
    # The timeout is the amount of time we wait until we tell the process to terminate
    # If you'd like to look at where the code is being executed, look in RunCommandAction.cpp -> runCommand()
    # RunCommandAction.cpp will log saying 'RunCommandAction has received termination command due to timeout'
    # (how long the wait is until the message is logged is based on the timeout in the command json file
    # Then RunCommandAction.cpp will call kill() on the process, there is no value passed into kill() so
    # the default wait time is used - found in ProcessImpl.cpp, DEFAULT_KILL_TIME
    # At the time of writing this test the value was 2 - this is the "Specified Amount Of Time"
    # hence checking that the log timings indicate a 2 second gap (+ some leeway)
    # The reason this test is excluded on coverage is because the DEFAULT_KILL_TIME changes
    wait_for_log_contains_from_mark    ${action_mark}    Entering cache result
    ${time_taken} =
        ...    get_time_difference_between_two_log_lines
        ...    Child process is still running, killing process
        ...    Entering cache result
        ...    ${action_mark}
    [Return]    ${time_taken}

RA Command Direct Suite setup
    Run Full Installer
    Install Response Actions Directly
    Copy Telemetry Config File in To Place
    Create File    ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [telemetry]\nVERBOSITY=DEBUG\n

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
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove File  ${EXE_CONFIG_FILE}


Stop MCSRouter
    ${result} =  Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Run Keyword If  ${result.rc} != 0  log  ${result.stdout}
    Run Keyword If  ${result.rc} != 0  log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0

Check sleep Running
    ${result} =    Run Process  pgrep    sleep
    Should Be Equal As Integers    ${result.rc}    0