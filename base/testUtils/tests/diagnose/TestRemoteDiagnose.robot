*** Settings ***

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/DiagnoseUtils.py


Library     Process
Library     OperatingSystem
Library     Collections


Resource    DiagnoseResources.robot
Resource    ../mcs_router/McsRouterResources.robot

Suite Setup  Require Fresh Install
Suite Teardown  Run Keywords
...             Ensure Uninstalled  AND
...             Cleanup Certificates

Test Teardown   Run Keywords
...             Stop Local Cloud Server
...             MCSRouter Default Test Teardown
Test Setup   Setup Fake Cloud
Default Tags  DIAGNOSE

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
        ...  10 secs
        ...  1 secs
        ...  Check Log Contains   Processing action    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log   Remote Diagnose

    Wait Until Keyword Succeeds
        ...  30 secs
        ...  10 secs
        ...  Directory Should Not Be Empty   ${SOPHOS_INSTALL}/base/remote-diagnose/output

    Wait Until Keyword Succeeds
        ...  20 secs
        ...  5 secs
        ...  Check Log Contains   Diagnose finished    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log   Remote Diagnose
    Wait Until Keyword Succeeds
        ...  20 secs
        ...  5 secs
        ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   mcsrouter  Sending status for SDU adapter   2

Test Remote Diagnose can handle two SDU actions
    Simulate SDU Action Now
    Simulate SDU Action Now
    Wait Until Keyword Succeeds
            ...  60 secs
            ...  10 secs
            ...  Check For Processed Tar Files


*** Keywords ***
check for processed tar files
    ${count} =  Count Files In Directory  ${SOPHOS_INSTALL}/base/remote-diagnose/output
    Should Be Equal As Integers  2   ${count}
Setup Fake Cloud
    Start Local Cloud Server  --initial-alc-policy  ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
    Generate Local Fake Cloud Certificates
    Regenerate Certificates
    Set Local CA Environment Variable

    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server


Simulate SDU Action Now
    Copy File   ${SUPPORT_FILES}/CentralXml/SDUAction.xml  ${SOPHOS_INSTALL}/tmp
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${SOPHOS_INSTALL}/tmp/SDUAction.xml    shell=True
    Should Be Equal As Integers    ${result.rc}    0  Failed to replace permission to file. Reason: ${result.stderr}
    Move File   ${SOPHOS_INSTALL}/tmp/SDUAction.xml  ${SOPHOS_INSTALL}/base/mcs/action/SDU_action_1.xml