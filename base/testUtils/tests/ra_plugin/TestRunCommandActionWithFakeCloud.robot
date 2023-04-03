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

Test Teardown      RA Run Command Test Teardown

Force Tags  LOAD5
Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Variables ***
${RESPONSE_JSON}        ${MCS_DIR}/response/CORE_id1_response.json

*** Test Cases ***
Test Run Command Action End To End With Fake Cloud
    Override LogConf File as Global Level  DEBUG

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
    Check Cloud Server Log Contains   {"duration":1,"exitCode":0,"stdErr":"","stdOut":""}
    Check Cloud Server Log Contains   {"duration":0,"exitCode":0,"stdErr":"","stdOut":""}
    Check Cloud Server Log Contains   sophos.mgt.response.RunCommands

    File Should exist  /tmp/test.txt
    ${test_contents} =  Get File    /tmp/test.txt
    Should Be Equal As Strings  ${test_contents}    two


*** Keywords ***

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