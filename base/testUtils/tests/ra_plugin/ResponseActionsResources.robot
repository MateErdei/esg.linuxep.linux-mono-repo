*** Settings ***
Library     Process

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot
*** Variables ***
${RESPONSE_ACTIONS_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/log/responseactions.log


*** Keywords ***
Install Response Actions Directly
    ${RESPONSE_ACTIONS_SDDS_DIR} =  Get SSPL Response Actions Plugin SDDS
    ${result} =    Run Process  bash -x ${RESPONSE_ACTIONS_SDDS_DIR}/install.sh 2> /tmp/install.log   shell=True
    ${error} =  Get File  /tmp/install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}, ${error}"
    Log  ${error}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Check Response Actions Executable Running

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
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  wait_for_log_contains_from_mark  ${mark}  responseactions <> Plugin Finished
    ${mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    Start Response Actions
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  wait_for_log_contains_from_mark  ${mark}  Entering the main loop

Check Response Actions Installed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains  Entering the main loop  ${RESPONSE_ACTIONS_LOG_PATH}  Response Actions log
    Check Response Actions Executable Running