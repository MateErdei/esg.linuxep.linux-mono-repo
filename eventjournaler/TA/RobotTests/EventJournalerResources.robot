*** Settings ***

*** Variables ***
${EVENT_JOURNALER_LOG_PATH}     ${EDR_PLUGIN_PATH}/log/eventjournaler.log
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
    ...  15 secs
    ...  1 secs
    ...  Marked File Contains  ${EVENT_JOURNALER_LOG_PATH}  eventjournaler <> Plugin Finished  ${mark}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Start Event Journaler
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Marked File Contains  ${EVENT_JOURNALER_LOG_PATH}  Plugin preparation complete  ${mark}
