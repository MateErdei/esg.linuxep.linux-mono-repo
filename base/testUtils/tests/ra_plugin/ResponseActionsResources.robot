*** Settings ***
Library     Process

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot

*** Variables ***
${RESPONSE_ACTIONS_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/log/responseactions.log
${ACTIONS_RUNNER_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/log/actionrunner.log
${RESPONSE_ACTIONS_TMP_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/tmp


*** Keywords ***
Install Response Actions Directly
    ${mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${RESPONSE_ACTIONS_SDDS_DIR} =  Get SSPL Response Actions Plugin SDDS
    ${result} =    Run Process  bash -x ${RESPONSE_ACTIONS_SDDS_DIR}/install.sh 2> /tmp/install.log   shell=True
    ${error} =  Get File  /tmp/install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}, ${error}"
    Log  ${error}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Response Actions Executable Running
    wait_for_log_contains_from_mark  ${mark}  Entering the main loop

Uninstall Response Actions
    ${result} =  Run Process     ${RESPONSE_ACTIONS_DIR}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

Check Response Actions Executable Running
    ${result} =    Run Process  pgrep responseactions | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check Response Actions Executable Not Running
    ${result} =    Run Process  pgrep  -a  responseactions
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Stop Response Actions
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  responseactions

Start Response Actions
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  responseactions

Restart Response Actions
    ${mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    Stop Response Actions
    wait_for_log_contains_from_mark  ${mark}  responseactions <> Plugin Finished   30
    ${mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    Start Response Actions
    wait_for_log_contains_from_mark  ${mark}  Entering the main loop  30

Check Response Actions Installed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains  Entering the main loop  ${RESPONSE_ACTIONS_LOG_PATH}  Response Actions log
    Check Response Actions Executable Running

RA Suite Teardown
    Stop Local Cloud Server
    Require Uninstalled

RA Suite Setup
    Start Local Cloud Server
    Regenerate Certificates
    Set Local CA Environment Variable
    Run Full Installer
    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag

RA Action With Register Suite Setup
    RA Suite Setup
    Register With Local Cloud Server

RA Test Setup
    Require Installed
    HttpsServer.Start Https Server  /tmp/cert.crt  443  tlsv1_2  True
    install_system_ca_cert  /tmp/cert.crt
    install_system_ca_cert  /tmp/ca.crt
    Install Response Actions Directly

RA Test Teardown
    General Test Teardown
    Run Cleanup Functions
    Uninstall Response Actions
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    cleanup_system_ca_certs
    Remove File  ${EXE_CONFIG_FILE}
    Run Keyword If Test Failed    Dump Cloud Server Log
    Remove File   /tmp/upload.zip
