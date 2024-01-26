*** Settings ***
Library     Collections
Library     OperatingSystem
Library     Process
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/DiagnoseUtils.py
Library     ${COMMON_TEST_LIBS}/HttpsServer.py
Library     ${COMMON_TEST_LIBS}/TelemetryUtils.py
Library     ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py
Library     ${COMMON_TEST_LIBS}/OnFail.py

Resource    ${COMMON_TEST_ROBOT}/DiagnoseResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Ensure Uninstalled

Test Teardown   Teardown

Test Setup   Setup Fake Cloud
Force Tags  DIAGNOSE    TAP_PARALLEL2

*** Variables ***
${HTTPS_LOG_FILE_PATH}     /tmp/https_server.log
*** Test Cases ***
Test Remote Diagnose can process SDU action with no URL field
    Test Remote Diagnose can process SDU action with malformed URL    SDUActionWithNoURLField.xml

Test Remote Diagnose can process SDU action with no URL value
    Test Remote Diagnose can process SDU action with malformed URL    SDUActionWithNoURLValue.xml

Test Remote Diagnose can process SDU action
    Override Local LogConf File for a component   DEBUG  global
    Run Process  systemctl  restart  sophos-spl
    Wait For Base Processes To Be Running Except MCS Router

    ${remote_diagnose_log_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    ${mcsrouter_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Simulate SDU Action Now
    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Processing action    140

    Register On Fail    Log Status Of Sophos Spl
    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Diagnose finished    30
    wait_for_log_contains_n_times_from_mark    ${mcsrouter_mark}      Sending status for SDU adapter    2

    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/remote-diagnose/output

Test Remote Diagnose can handle two SDU actions
    [Timeout]  7 minutes
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  sdu
    ${remote_diagnose_log_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  sdu
    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Entering the main loop    10

    ${mcsrouter_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Simulate SDU Action Now  action_suffix=1
    Simulate SDU Action Now  action_suffix=2
    wait_for_log_contains_n_times_from_mark    ${mcsrouter_mark}      Sending status for SDU adapter    2    320
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/remote-diagnose/output



*** Keywords ***
check for processed tar files
    ${count} =  Count Files In Directory  ${SOPHOS_INSTALL}/base/remote-diagnose/output
    Should Be Equal As Integers  2   ${count}
Setup Fake Cloud
    Start Local Cloud Server
    Setup_MCS_Cert_Override
    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server
    HttpsServer.Start Https Server  ${COMMON_TEST_UTILS}/server_certs/server.crt    443  tlsv1_2


Teardown
    Run Teardown Functions
    Stop Local Cloud Server
    MCSRouter Default Test Teardown
    Stop Https Server
    Dump Teardown Log  ${HTTPS_LOG_FILE_PATH}
    Remove File  ${HTTPS_LOG_FILE_PATH}

Simulate SDU Action Now
    [Arguments]  ${action_suffix}=1  ${action_xml_file_name}=SDUAction.xml
    Copy File   ${SUPPORT_FILES}/CentralXml/${action_xml_file_name}  ${MCS_TMP_DIR}
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${MCS_TMP_DIR}/${action_xml_file_name}   shell=True
    Should Be Equal As Integers    ${result.rc}    0  Failed to replace permission to file. Reason: ${result.stderr}
    Move File   ${MCS_TMP_DIR}/${action_xml_file_name}    ${MCS_DIR}/action/SDU_action_${action_suffix}.xml

Test Remote Diagnose can process SDU action with malformed URL
    [Arguments]    ${input_action_xml_file_name}
    Override Local LogConf File for a component   DEBUG  global
    Run Process  systemctl  restart  sophos-spl
    Wait For Base Processes To Be Running Except MCS Router

    ${remote_diagnose_log_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    ${mcsrouter_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

    Simulate SDU Action Now  action_xml_file_name=${input_action_xml_file_name}

    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Processing action    140
    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Cannot process url will not send up diagnose file Error: Malformed url missing protocol    30
    wait_for_log_contains_n_times_from_mark    ${mcsrouter_mark}      Sending status for SDU adapter    2
