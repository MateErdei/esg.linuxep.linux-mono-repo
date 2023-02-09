*** Settings ***

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/DiagnoseUtils.py
Library     ${LIBS_DIRECTORY}/HttpsServer.py
Library     ${LIBS_DIRECTORY}/TelemetryUtils.py



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
Default Tags  DIAGNOSE
*** Variables ***
${HTTPS_LOG_FILE_PATH}     /tmp/https_server.log
*** Test Cases ***
Test Remote Diagnose can process SDU action
    Override Local LogConf File for a component   DEBUG  global
    Run Process  systemctl  restart  sophos-spl
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  1 secs
        ...  Check Expected Base Processes Are Running

    Simulate SDU Action Now
    Wait Until Keyword Succeeds
        ...  140 secs
        ...  1 secs
        ...  Check Log Contains   Processing action    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log   Remote Diagnose

    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  Check Log Contains   Diagnose finished    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log   Remote Diagnose
    Wait Until Keyword Succeeds
        ...  40 secs
        ...  5 secs
        ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   mcsrouter  Sending status for SDU adapter   2

    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/remote-diagnose/output

Test Remote Diagnose can handle two SDU actions
    [Timeout]  7 minutes
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  sdu
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  sdu
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Entering the main loop  ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log  SDU Log

    Simulate SDU Action Now  action_suffix=1
    Simulate SDU Action Now  action_suffix=2
    Wait Until Keyword Succeeds
    ...  320 secs
    ...  5 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log   Remote Diagnose  Diagnose finished   2
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
    Stop Local Cloud Server
    MCSRouter Default Test Teardown
    Stop Https Server
    Dump Teardown Log  ${HTTPS_LOG_FILE_PATH}
    Remove File  ${HTTPS_LOG_FILE_PATH}
    cleanup_system_ca_certs

Simulate SDU Action Now
    [Arguments]  ${action_suffix}=1
    Copy File   ${SUPPORT_FILES}/CentralXml/SDUAction.xml  ${SOPHOS_INSTALL}/tmp
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${SOPHOS_INSTALL}/tmp/SDUAction.xml    shell=True
    Should Be Equal As Integers    ${result.rc}    0  Failed to replace permission to file. Reason: ${result.stderr}
    Move File   ${SOPHOS_INSTALL}/tmp/SDUAction.xml  ${SOPHOS_INSTALL}/base/mcs/action/SDU_action_${action_suffix}.xml