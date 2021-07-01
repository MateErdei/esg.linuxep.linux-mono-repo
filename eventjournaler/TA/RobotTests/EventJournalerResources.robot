*** Settings ***
Library         Process
Library         OperatingSystem
Library         String

Resource  GeneralResources.robot
*** Variables ***
${EVENT_JOURNALER_LOG_PATH}     ${COMPONENT_ROOT_PATH}/log/eventjournaler.log
${BASE_SDDS}                    ${TEST_INPUT_PATH}/base_sdds/
${EVENT_JOURNALER_SDDS}         ${COMPONENT_SDDS}

*** Keywords ***
Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Shell Process   bash -x ${BASE_SDDS}/install.sh 2> /tmp/installer.log   OnError=Failed to Install Base   timeout=60s
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install Event Journaler Directly from SDDS
    ${result} =   Run Process  bash ${EVENT_JOURNALER_SDDS}/install.sh   shell=True   timeout=120s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install eventjournaler.\nstdout: \n${result.stdout}\n. stderr: \n{result.stderr}"

Uninstall Base
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   shell=True   timeout=30s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall base.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"
    
Uninstall Event Journaler
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/plugins/eventjournaler/bin/uninstall.sh --force   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall Event Journaler.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"

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

Check Event Journaler Log contains
    [Arguments]  ${string_to_contain}
    File Should Contain  ${EVENT_JOURNALER_LOG_PATH}  ${string_to_contain}

Check Marked Event Journaler Log contains Contains
    [Arguments]  ${input}  ${mark}
    Marked File Contains  ${EVENT_JOURNALER_LOG_PATH}   ${input}   ${mark}

Event Journaler Teardown
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Run Keyword If Test Failed  Log File  ${EVENT_JOURNALER_LOG_PATH}

Journaler Log Contains String X Times
    [Arguments]  ${to_find}   ${xtimes}
    ${content} =  Get File   ${EVENT_JOURNALER_LOG_PATH}
    Should Contain X Times  ${content}  ${to_find}  ${xtimes}

Wait Until Journaler Log Contains String X Times
    [Arguments]  ${to_find}  ${xtimes}  ${waitSeconds}=25
    Wait Until Keyword Succeeds
    ...  ${waitSeconds} secs
    ...  2 secs
    ...  Journaler Log Contains String X Times  ${to_find}  ${xtimes}

Marked Journaler Log Contains String X Times
    [Arguments]  ${input}   ${xtimes}  ${mark}
    Marked File Contains X Times  ${EVENT_JOURNALER_LOG_PATH}  ${input}  ${xtimes}  ${mark}

Wait Until Marked Journaler Log Contains String X Times
    [Arguments]  ${to_find}  ${xtimes}  ${mark}  ${waitSeconds}=25
    Wait Until Keyword Succeeds
    ...  ${waitSeconds} secs
    ...  2 secs
    ...  Marked Journaler Log Contains String X Times  ${to_find}  ${xtimes}  ${mark}
