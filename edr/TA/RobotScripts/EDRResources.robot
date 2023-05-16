*** Settings ***
Library         Process
Library         OperatingSystem
Library         String
Library         DateTime

Library         ../Libs/OSLibs.py
Library         ../Libs/XDRLibs.py

Resource    ComponentSetup.robot

*** Variables ***
${EDR_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${EDR_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${EDR_LOG_PATH}     ${EDR_PLUGIN_PATH}/log/edr.log
${EDR_LOG_DIR}      ${EDR_PLUGIN_PATH}/log
${SCHEDULED_QUERY_LOG_PATH}     ${EDR_PLUGIN_PATH}/log/scheduledquery.log
${LIVEQUERY_LOG_PATH}     ${EDR_PLUGIN_PATH}/log/livequery.log
${BASE_SDDS}        ${TEST_INPUT_PATH}/base_sdds/
${EDR_SDDS}         ${COMPONENT_SDDS}

*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\nstdout: \n${result.stdout} \n. stderr: \n${result.stderr}"

Check EDR Plugin Running
    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep ${COMPONENT_NAME}
    Should Be Equal As Integers    ${rc}    0    EDR not running

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

Marked File Does Not Contain
    [Arguments]  ${path}  ${input}  ${mark}
    ${content} =  Get File   ${path}
    ${content} =  Evaluate  "\\n".join(${content.__repr__()}.split("\\n")[${mark}:])
    Should Not Contain  ${content}  ${input}

File Log Contains
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}
    Should Contain  ${content}  ${input}

File Log Does Not Contain
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}
    Should Not Contain  ${content}  ${input}

Scheduled Query Log Contains
    [Arguments]  ${input}
    File Log Contains  ${SCHEDULED_QUERY_LOG_PATH}   ${input}

EDR Plugin Log Contains
    [Arguments]  ${input}
    File Log Contains  ${EDR_LOG_PATH}   ${input}

EDR Plugin Log Does Not Contain
    [Arguments]  ${input}
    File Log Does Not Contain  ${EDR_LOG_PATH}   ${input}

LiveQuery Log Contains
    [Arguments]  ${input}
    File Log Contains  ${LIVEQUERY_LOG_PATH}   ${input}

EDR Plugin Log Contains X Times
    [Arguments]  ${input}   ${xtimes}
    ${content} =  Get File   ${EDR_LOG_PATH}
    Should Contain X Times  ${content}  ${input}  ${xtimes}

FakeManagement Log Contains
    [Arguments]  ${input}
    File Log Contains  ${FAKEMANAGEMENT_AGENT_LOG_PATH}   ${input}

Check EDR Plugin Installed
    File Should Exist   ${EDR_PLUGIN_PATH}/bin/edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check EDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  edr <> Entering the main loop
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  FakeManagement Log Contains   Registered plugin: edr

Simulate Live Query
    [Arguments]  ${name}  ${correlation}=123-4
    ${requestFile} =  Set Variable  ${EXAMPLE_DATA_PATH}/${name}
    File Should Exist  ${requestFile}
    Copy File   ${requestFile}  ${SOPHOS_INSTALL}/tmp
    Run Shell Process  chown sophos-spl-user:sophos-spl-group ${SOPHOS_INSTALL}/tmp/${name}  OnError=Failed to change ownership
    ${creation_time} =  Get Current Date
    ${eptime} =  Get Time  epoch
    Log To Console  ${eptime}
    ${delta} =  Set Variable   10000
    ${ttl} =  Evaluate   ${eptime}+${delta}
    Move File    ${SOPHOS_INSTALL}/tmp/${name}  ${SOPHOS_INSTALL}/base/mcs/action/LiveQuery_${correlation}_request_${creation_time}_${ttl}.json

Install With Base SDDS
    [Arguments]  ${enableAuditConfig}=False  ${preInstallALCPolicy}=False
    Install With Base SDDS Inner  ${enableAuditConfig}  ${preInstallALCPolicy}
    Install EDR Directly from SDDS

Install With Base SDDS Debug
    [Arguments]  ${enableAuditConfig}=False  ${preInstallALCPolicy}=False
    Install With Base SDDS Inner  ${enableAuditConfig}  ${preInstallALCPolicy}
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create Debug Level Logger Config File


Install With Base SDDS Inner
    [Arguments]  ${enableAuditConfig}=False  ${preInstallALCPolicy}=False
    Uninstall All
    Directory Should Not Exist  ${SOPHOS_INSTALL}
    Install Base For Component Tests
    Run Keyword If  ${enableAuditConfig}  Create Install Options File With Content  --disable-auditd
    ${ALCContent}=  Get ALC Policy Without MTR
    Run Keyword If  ${preInstallALCPolicy}  Install ALC Policy   ALCContent

Uninstall EDR
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/plugins/edr/bin/uninstall.sh --force   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall EDR.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"
    [Return]  ${result.stdout}  ${result.stderr}

Uninstall And Revert Setup
    Uninstall All
    Setup Base And Component

Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Shell Process   bash -x ${BASE_SDDS}/install.sh 2> /tmp/installer.log   OnError=Failed to Install Base   timeout=60s
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install EDR Directly from SDDS
    [Arguments]  ${interval}=5
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.mtr.conf  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.mtr.conf  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf       ${interval}
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf            ${interval}
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf   ${interval}
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf        ${interval}
    Remove Discovery Query From Pack  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    Remove Discovery Query From Pack  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=120s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install edr.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"
    [Return]  ${result.stdout}

Install EDR Directly from SDDS With mocked scheduled queries
    [Arguments]  ${interval}=5
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.mtr.conf  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.mtr.conf  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf       ${interval}
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf            ${interval}
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf   ${interval}
    Change All Scheduled Queries Interval  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf        ${interval}
    Replace Query Bodies With Sql That Always Gives Results  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf
    Replace Query Bodies With Sql That Always Gives Results  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf
    Replace Query Bodies With Sql That Always Gives Results  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    Replace Query Bodies With Sql That Always Gives Results  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    Remove Discovery Query From Pack  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    Remove Discovery Query From Pack  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    Copy File  ${EXAMPLE_DATA_PATH}/testoptions.conf   ${EDR_SDDS}/etc/osquery.conf.d/testoptions.conf
    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=120s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install edr.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"
    [Return]  ${result.stdout}\n${result.stderr}

Install EDR Directly from SDDS With Latest And Next Marked
    Create File  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf  {"schedule": {"latest_xdr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf  {"schedule": {"latest_mtr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf  {"schedule": {"next_xdr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf  {"schedule": {"next_mtr_query": {"query": "select * from uptime;","interval": 2}}}
    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=120s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install edr.\nstdout: \n${result.stdout}\n. stderr: \n{result.stderr}"


Install EDR Directly from SDDS With Fixed Value Queries
    Create File  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf  { "schedule": { "uptime": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" }, "uptime_not_folded": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf  { "schedule": { "uptime": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" }, "uptime_not_folded": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf  { "schedule": { "uptime": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" }, "uptime_not_folded": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf  { "schedule": { "uptime": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" }, "uptime_not_folded": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }

    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=120s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install edr.\nstdout: \n${result.stdout}\n. stderr: \n{result.stderr}"

Install EDR Directly from SDDS With Random Queries
    Create File  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.conf              { "schedule": { "random": { "query": "SELECT abs(random() % 2) AS number;", "interval": 1, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${EDR_SDDS}/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf          { "schedule": { "random": { "query": "SELECT abs(random() % 2) AS number;", "interval": 1, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.conf         { "schedule": { "random": { "query": "SELECT abs(random() % 2) AS number;", "interval": 1, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${EDR_SDDS}/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf     { "schedule": { "random": { "query": "SELECT abs(random() % 2) AS number;", "interval": 1, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }

    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=120s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install edr.\nstdout: \n${result.stdout}\n. stderr: \n{result.stderr}"

Check EDR Plugin Installed With Base
    File Should Exist   ${EDR_PLUGIN_PATH}/bin/edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check EDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  edr <> Entering the main loop
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Plugin preparation complete
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Log Contains  SophosExtension running


Get ALC Policy Without MTR
    ${alc} =  Get File  ${EXAMPLE_DATA_PATH}/ALC_policy_with_mtr_example.xml
    ${alc_without_mtr} =  Replace String  ${alc}  <Feature id="MDR"/>   ${EMPTY}
    [Return]  ${alc_without_mtr}


Install ALC Policy
  [Arguments]  ${ALCContent}
  ${file_path}=  Set Variable  ${SOPHOS_INSTALL}/tmp/ALC-1_policy.xml
  Create File   ${file_path}  ${ALCContent}
  Run Process  chown sophos-spl-user:sophos-spl-group ${file_path}  shell=True
  Move File   ${file_path}      ${SOPHOS_INSTALL}/base/mcs/policy


Check Osquery Running
    ${rc}   ${pid} =    Run And Return Rc And Output    pgrep osqueryd
    Should Be Equal As Integers    ${rc}    0    osquery not running

Check Osquery Not Running
    ${result} =    Run Process  pgrep  -a  osqueryd
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Display All SSPL Files Installed
    ${handle}=  Start Process  find ${SOPHOS_INSTALL} | grep -v python | grep -v comms | grep -v primarywarehouse | grep -v comms | grep -v temp_warehouse | grep -v TestInstallFiles | grep -v lenses   shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

Log Status Of Sophos Spl
    ${result} =  Run Process    systemctl  status  sophos-spl
    Log  ${result.stdout}
    ${result} =  Run Process    systemctl  status  sophos-spl-update
    Log  ${result.stdout}
    ${result} =  Run Process    systemctl  status  sophos-spl-diagnose
    Log  ${result.stdout}

Common Teardown
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log Status Of Sophos Spl
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File  ${COMPONENT_ROOT_PATH}/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File  ${COMPONENT_ROOT_PATH}/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File  ${COMPONENT_ROOT_PATH}/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File  ${COMPONENT_ROOT_PATH}/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf.DISABLED
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/plugins/edr/extensions/extensions.load
    Run Keyword If Test Failed  Get all sophos processes
    Run Keyword If Test Failed  Log File  ${EDR_LOG_PATH}
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File  ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${LIVEQUERY_LOG_PATH}
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log
    Run Keyword If Test Failed  Display All SSPL Files Installed

EDR And Base Teardown
    Stop EDR
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check EDR Executable Not Running
    Common Teardown
    Remove File    ${EDR_LOG_PATH}
    Start EDR

Create Install Options File With Content
    [Arguments]  ${installFlags}
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  ${installFlags}
    Run Process  chown  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/install_options  shell=True
    Run Process  chmod  440  ${SOPHOS_INSTALL}/base/etc/install_options  shell=True

Check Ownership
    [Arguments]  ${path}  ${owner}
    ${result} =  Run Process  ls  -l  ${path}
    Should Contain  ${result.stdout}  ${owner}

Check EDR Executable Running
    ${result} =    Run Process  pgrep edr | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check EDR Executable Not Running
    ${result} =    Run Process  pgrep  -a  edr
    Run Keyword If  ${result.rc}==0   Get edr process info
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Get Osquery pid
    ${edr_osquery_pid} =    Run Process  pgrep -a osquery | grep plugins/edr | grep -v osquery.conf | head -n1 | cut -d " " -f1  shell=true
    [Return]  ${edr_osquery_pid.stdout}

Get edr process info
    ${result} =  Run Process  ps -ef | grep edr  shell=true
    Log  ${result.stdout}

Get all sophos processes
    ${result} =  Run Process  ps -ef | grep sophos  shell=true
    Log  ${result.stdout}

Restart EDR
    ${mark} =  Mark File  ${EDR_LOG_PATH}
    Stop EDR
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Marked File Contains  ${EDR_LOG_PATH}  edr <> Plugin Finished  ${mark}
    ${mark} =  Mark File  ${EDR_LOG_PATH}
    Start EDR
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Marked File Contains  ${EDR_LOG_PATH}  Plugin preparation complete  ${mark}

File Should Contain
    [Arguments]  ${file}  ${string_to_contain}
    ${content} =  Get File  ${file}
    Should Contain  ${content}   ${string_to_contain}

File Should Contain Only
    [Arguments]  ${file}  ${string}
    ${content} =  Get File  ${file}
    Should Be Equal As Strings  ${content}   ${string}

File Should Not Contain Only
    [Arguments]  ${file}  ${string}
    ${content} =  Get File  ${file}
    Should Not Be Equal As Strings  ${content}   ${string}

Move File Atomically
    [Arguments]  ${source}  ${destination}
    Copy File  ${source}  /opt/NotARealFile
    Move File  /opt/NotARealFile  ${destination}

Move File Atomically as sophos-spl-local
    [Arguments]  ${source}  ${destination}
    Copy File  ${source}  /opt/NotARealFile
    Run Process  chown  sophos-spl-local:sophos-spl-group  /opt/NotARealFile
    Run Process  chmod  640  /opt/NotARealFile
    Move File  /opt/NotARealFile  ${destination}

Enable XDR
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Is XDR Enabled in Plugin Conf

Enable XDR with MTR
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled_with_MTR.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Is XDR Enabled in Plugin Conf

Is XDR Enabled in Plugin Conf
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  EDR Plugin Log Contains   Updating running_mode flag setting
    # race condition here between the above log and the file being written
    sleep  0.1s
    ${EDR_CONFIG_CONTENT}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   running_mode=1

Stop EDR
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr  timeout=35s

Start EDR
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr

Apply Live Query Policy And Wait For Query Pack Changes
    [Arguments]  ${policy}
    ${mark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Move File Atomically  ${policy}  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  LiveQuery policy has changed. Restarting osquery to apply changes  ${mark}
    Wait Until Keyword Succeeds
    ...  20s
    ...  2s
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Plugin preparation complete  ${mark}

Create Debug Level Logger Config File
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n

Run Live Query and Return Result
    [Arguments]  ${query}=SELECT name,value from osquery_flags where name == 'disable_audit'
    ${query_template} =  Get File  ${EXAMPLE_DATA_PATH}/GenericQuery.json
    ${query_json} =  Replace String  ${query_template}  %QUERY%  ${query}
    Log  ${query_json}
    Create File  ${EXAMPLE_DATA_PATH}/temp.json  ${query_json}
    ${response}=  Set Variable  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_123-4_response.json
    Simulate Live Query  temp.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${response}
    ${contents} =  Get File  ${response}
    Remove File  ${response}
    Remove File  ${EXAMPLE_DATA_PATH}/temp.json
    [Return]  ${contents}

Replace Rsyslog Apparmor Rule
    ${apparmor_profile} =    Set Variable    /etc/apparmor.d/usr.sbin.rsyslogd
    Copy File If Destination Missing  ${apparmor_profile}  ${apparmor_profile}_bkp
    Create File    ${apparmor_profile}   /usr/sbin/rsyslogd {\n /opt/sophos-spl/shared/syslog_pipe rw,\n}\n
    ${result} =   Run Process    apparmor_parser  -r  ${apparmor_profile}
    Log  ${result.stdout}
    Log  ${result.stderr}

Restore Rsyslog Apparmor Rule
    ${apparmor_profile} =    Set Variable    /etc/apparmor.d/usr.sbin.rsyslogd
    Move File  ${apparmor_profile}_bkp  ${apparmor_profile}
    ${result} =   Run Process    apparmor_parser  -r  ${apparmor_profile}
    Log  ${result.stdout}
    Log  ${result.stderr}