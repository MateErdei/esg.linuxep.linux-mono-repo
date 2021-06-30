*** Settings ***
Library     Process

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot
*** Variables ***
${EVENT_JOURNALER_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/eventjournaler/log/eventjournaler.log


*** Keywords ***
Install Event Journaler Directly
    ${EVENT_JOURNALER_SDDS_DIR} =  Get SSPL Event Journaler Plugin SDDS
    ${result} =    Run Process  bash -x ${EVENT_JOURNALER_SDDS_DIR}/install.sh 2> /tmp/install.log   shell=True
    ${error} =  Get File  /tmp/install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}, ${error}"
    Log  ${error}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Check Event Journaler Executable Running

Check Event Journaler Executable Running
    ${result} =    Run Process  pgrep eventjournaler | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check Event Journaler Executable Not Running
    ${result} =    Run Process  pgrep  -a  eventjournaler
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Stop Event Journaler
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop eventjournaler   OnError=failed to stop eventjournaler  timeout=35s

Start Event Journaler
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start eventjournaler   OnError=failed to start eventjournaler

Restart Event Journaler
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Stop Event Journaler
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Marked File Contains  ${EVENT_JOURNALER_LOG_PATH}  eventjournaler <> Plugin Finished  ${mark}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Start Event Journaler
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Marked File Contains  ${EVENT_JOURNALER_LOG_PATH}  Entering the main loop  ${mark}

Mark File
    [Arguments]  ${path}
    ${content} =  Get File   ${path}
    Log  ${content}
    [Return]  ${content.split("\n").__len__()}

Marked File Contains
    [Arguments]  ${path}  ${input}  ${mark}
    ${content} =  Get File   ${path}
    ${content} =  Evaluate  "\\n".join(${content.__repr__()}.split("\\n")[${mark}:])
    Should Contain  ${content}  ${input}

Check Event Journaler Installed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains  Entering the main loop  ${EVENT_JOURNALER_LOG_PATH}  event journaler log
    Check Event Journaler Executable Running


