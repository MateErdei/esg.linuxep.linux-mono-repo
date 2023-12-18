*** Settings ***
Library     Process
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py

Resource    GeneralTeardownResource.robot

*** Variables ***
${EVENT_JOURNALER_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/eventjournaler/log/eventjournaler.log
${EVENT_READER_TOOL}          ${EVENT_JOURNALER_TOOLS}/JournalReader
${COMPONENT_ROOT_PATH}        ${SOPHOS_INSTALL}/plugins/eventjournaler
${EVENT_JOURNAL_DIR}          ${COMPONENT_ROOT_PATH}/data/eventjournals
${JOURNALED_EICAR}            {"avScanType":203,"details":{"filePath":"/tmp/dirty_excluded_file","sha256FileHash":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"/tmp/dirty_excluded_file","primary":true,"sha256":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","type":1}},"quarantineSuccess":true,"threatSource":1,"threatType":1,"time":
${JOURNALED_EICAR2}           {"avScanType":203,"details":{"filePath":"/tmp/dirty_excluded_file2","sha256FileHash":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"/tmp/dirty_excluded_file2","primary":true,"sha256":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","type":1}},"quarantineSuccess":true,"threatSource":1,"threatType":1,"time":


*** Keywords ***
Install Event Journaler Directly
    ${EVENT_JOURNALER_SDDS_DIR} =  Get SSPL Event Journaler Plugin SDDS
    ${result} =    Run Process  bash -x ${EVENT_JOURNALER_SDDS_DIR}/install.sh 2> /tmp/install.log   shell=True
    ${error} =  Get File  /tmp/install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}, ${error}"
    Log  ${error}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Check Event Journaler Installed

Uninstall Event Journaler
    ${result} =  Run Process     ${COMPONENT_ROOT_PATH}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

Check Event Journaler Executable Running
    ${result} =    Run Process  pgrep eventjournaler | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check Event Journaler Executable Not Running
    ${result} =    Run Process  pgrep  -a  eventjournaler
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Stop Event Journaler
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  eventjournaler

Start Event Journaler
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  eventjournaler

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
    ...  Marked File Contains  ${EVENT_JOURNALER_LOG_PATH}  Completed initialization of Event Journaler  ${mark}

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
    ...  10 secs
    ...  1 secs
    ...  Check Event Journaler Executable Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains  Completed initialization of Event Journaler  ${EVENT_JOURNALER_LOG_PATH}  event journaler log

Read First Event From Journal
    ${result1} =   Run Process  chmod  +x  ${EVENT_READER_TOOL}
    Should Be Equal As Integers  ${result1.rc}  0    Chmod failed with: ${result1.rc}
    ${result}=    Run Process  ls -l /opt/test/inputs/  shell=True
    Log  ${result.stdout}
    ${result}=    Run Process  ls -l /opt/test/inputs/ej_manual_tools/  shell=True
    Log  ${result.stdout}
    ${result2} =   Run Process  ${EVENT_READER_TOOL}  -l  ${EVENT_JOURNAL_DIR}/  -s  Detections  -p  SophosSPL  -u  -t  0  --flush-delay-disable  -c  1
    log to console   ${result2.stdout}
    log  ${result2.stdout}
    log  ${result2.stderr}
    Should Be Equal As Integers  ${result2.rc}  0    Event read process failed with: ${result2.rc}
    [Return]   ${result2.stdout}

Check Journal Contains Detection Event With Content
    [Arguments]  ${expectedContent}
    ${latestJournalEvent} =  Read All Detection Events From Journal
    Should Contain  ${latestJournalEvent}   ${expectedContent}

Check Journal Is Empty
    ${JournalEvent} =  Read First Event From Journal
    Should Be Empty  ${JournalEvent}

Check Journal Contains X Detection Events
    [Arguments]  ${xtimes}
    ${all_events} =  Read All Detection Events From Journal
    Should Contain X Times  ${all_events}  Producer Unique Id:  ${xtimes}

Read All Detection Events From Journal
    ${result} =   Run Process  ${EVENT_READER_TOOL}  -l  ${EVENT_JOURNAL_DIR}/  -s  Detections  -p  SophosSPL  -u  -t  0  --flush-delay-disable
    log to console   ${result.stdout}
    log  ${result.stdout}
    log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0
    [Return]   ${result.stdout}