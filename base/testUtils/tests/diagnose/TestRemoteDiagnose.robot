*** Settings ***

Library     ../../libs/LogUtils.py
Library     ../../libs/DiagnoseUtils.py
Library     ../../libs/HttpsServer.py
Library     ../../libs/TelemetryUtils.py



Library     Process
Library     OperatingSystem
Library     Collections


Resource    DiagnoseResources.robot
Resource    ../mcs_router/McsRouterResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Run Keywords
...             Ensure Uninstalled  AND
...             Cleanup Certificates

Test Teardown   Teardown

Test Setup   Setup Fake Cloud
Default Tags  DIAGNOSE    TAP_TESTS
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
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  1 secs
        ...  Check Expected Base Processes Are Running

    ${remote_diagnose_log_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    ${mcsrouter_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Simulate SDU Action Now
    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Processing action    140
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
    Generate Local Fake Cloud Certificates
    Regenerate Certificates
    Set Local CA Environment Variable

    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server

    HttpsServer.Start Https Server  /tmp/cert.crt  443  tlsv1_2  True
    install_system_ca_cert  /tmp/cert.crt
    install_system_ca_cert  /tmp/ca.crt

Teardown
    log file    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Stop Local Cloud Server
    MCSRouter Default Test Teardown
    Stop Https Server
    Dump Teardown Log  ${HTTPS_LOG_FILE_PATH}
    Remove File  ${HTTPS_LOG_FILE_PATH}
    cleanup_system_ca_certs

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
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  1 secs
        ...  Check Expected Base Processes Are Running

    ${remote_diagnose_log_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    ${mcsrouter_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

    Simulate SDU Action Now  action_xml_file_name=${input_action_xml_file_name}

    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Processing action    140
    wait_for_log_contains_from_mark    ${remote_diagnose_log_mark}    Cannot process url will not send up diagnose file Error: Malformed url missing protocol    30
    wait_for_log_contains_n_times_from_mark    ${mcsrouter_mark}      Sending status for SDU adapter    2
    