*** Settings ***
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py

Resource    GeneralUtilsResources.robot
Resource    UpgradeResources.robot


*** Variables ***
#EDR Telemetry variables
${COMPONENT_TEMP_DIR}  /tmp/edr_component
${CRASH_QUERY} =  WITH RECURSIVE counting (curr, next) AS ( SELECT 1,1 UNION ALL SELECT next, curr+1 FROM counting LIMIT 10000000000 ) SELECT group_concat(curr) FROM counting;
${SOPHOS_INFO_QUERY} =  SELECT endpoint_id from sophos_endpoint_info;
${SIMPLE_QUERY_1_ROW} =  SELECT * from users limit 1;
${GREP} =  SELECT * FROM grep WHERE path = '/opt/sophos-spl/plugins/edr/VERSION.ini' AND pattern = 'VERSION';
${EXTENSION_CRASH} =  SELECT * FROM memorycrashtable;
${SIMPLE_QUERY_2_ROW} =  SELECT * from users limit 2;
${SIMPLE_QUERY_4_ROW} =  SELECT * from users limit 4;
${EDR_PLUGIN_PATH}    ${SOPHOS_INSTALL}/plugins/edr
${EDR_LOG_PATH} =     ${EDR_PLUGIN_PATH}/log/edr.log


*** Keywords ***

Wait For EDR to be Installed
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   File Should exist    ${SOPHOS_INSTALL}/plugins/edr/bin/edr

    Wait Until Keyword Succeeds
    ...   40 secs
    ...   2 secs
    ...   EDR Plugin Is Running

    Wait Until Keyword Succeeds
    ...   40 secs
    ...   2 secs
    ...   Check EDR Osquery Executable Running
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   File Should exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/options.conf
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   Check Marked EDR Log Contains   Completed initialisation of EDR

Wait For Sophos Extension To Be Initialised
    [Arguments]  ${times}=1
    Wait Until Keyword Succeeds
    ...  45
    ...  5
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/edr.log   edr_log  SophosExtension running  ${times}

Wait For Sophos Extension To Be Initialised Including OSQuery Restarts
    Wait For Sophos Extension To Be Initialised    times=2
    # If "Osquery health check failed" in logs, OSQuery will restart and we need to wait again
    ${status}  ${value} =  Run Keyword And Ignore Error   Check Log Does Not Contain    Osquery health check failed    ${SOPHOS_INSTALL}/plugins/edr/log/edr.log    edr_log
    Run Keyword If  '${status}' == 'FAIL'  Wait For Sophos Extension To Be Initialised    times=3


EDR Plugin Is Running
    ${result} =    Run Process  pgrep  edr
    Should Be Equal As Integers    ${result.rc}    0

EDR Plugin Is Not Running
    ${result} =    Run Process  pgrep  edr
    Should Not Be Equal As Integers    ${result.rc}    0   EDR PLugin still running

MTR Extension is running
    ${result} =    Run Process  pgrep  SophosMTR.ext
    Should Be Equal As Integers    ${result.rc}    0

Restart EDR Plugin
    [Arguments]  ${clearLog}=False    ${installQueryPacks}=False
    Mark EDR Log
    Wdctl Stop Plugin  edr
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   Check Marked EDR Log Contains   Plugin Finished
    Log File  ${EDR_DIR}/log/edr.log
    Run Keyword If   ${clearLog}   Remove File  ${EDR_DIR}/log/edr.log
    Run Keyword If   ${installQueryPacks}   Create Query Packs
    Mark EDR Log
    # If rsyslog restart causes sophos-spl restart, wait for it to start
    ${spl_restarting} =    Journalctl Contains    sophos-spl.service: Killing process
    Run Keyword If   ${spl_restarting}   Wdctl Stop Plugin  edr
    Wdctl Start Plugin  edr
    Wait Until Keyword Succeeds
    ...  240 secs
    ...  1 secs
    ...  Check Marked EDR Log Contains   Plugin preparation complete

Cloud Server Is Running
    ${result} =  Run Process  curl -k https://localhost:4443/mcs   shell=True
    Should Be Equal As Integers  ${result.rc}  0  "Failed to Verify connection to Fake Cloud\nStdOut: ${result.stdout}\nStdErr: ${result.stderr}"

Install EDR SDDS3
    [Arguments]  ${policy}  ${args}=${None}
    Start Local Cloud Server  --initial-alc-policy  ${policy}
    Wait Until Keyword Succeeds
        ...   20 secs
        ...   1 secs
        ...   Cloud Server Is Running
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080

    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150

    Wait For EDR to be Installed

    Should Exist  ${EDR_DIR}

Run Query Until It Gives Expected Results
    [Arguments]  ${query}  ${expectedResponseJson}
    Send Query From Fake Cloud    ProcessEvents  ${query}  command_id=queryname
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Last Live Query Response   ${expectedResponseJson}

Report On MCS_CA
    ${mcs_ca} =  Get Environment Variable  MCS_CA
    ${result}=  Run Process    ls -l ${mcs_ca}     shell=True
    Log  ${result.stdout}
    Log File  ${mcs_ca}

Check EDR Log Contains
    [Arguments]  ${string_to_contain}
    ${EDR_LOG_CONTENT}=  Get File  ${EDR_DIR}/log/edr.log
    Should Contain  ${EDR_LOG_CONTENT}   ${string_to_contain}

Check EDR OSQuery Log Contains
        [Arguments]  ${string_to_contain}
        ${EDR_OSQUERY_LOG_CONTENT}=  Get File  ${EDR_DIR}/log/edr_osquery.log
        Should Contain  ${EDR_OSQUERY_LOG_CONTENT}   ${string_to_contain}

Check Scheduled Query Log Contains
    [Arguments]  ${string_to_contain}
    ${SCHEDULED_QUERY_LOG_CONTENT}=  Get File  ${EDR_DIR}/log/scheduledquery.log
    Should Contain  ${SCHEDULED_QUERY_LOG_CONTENT}   ${string_to_contain}


EDR Suite Setup
    Upgrade Resources Suite Setup
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Set Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL  1
    # LINUXDAR-7059: On SUSE the thin installer fails to connect to the first SDDS3 server so workaround for now by running twice
    ${result} =  Run Process    bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${COMMON_TEST_LIBS}/SDDS3server.py --launchdarkly /tmp/launchdarkly --sdds3 ${VUT_WAREHOUSE_ROOT}  shell=true    timeout=10s
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${COMMON_TEST_LIBS}/SDDS3server.py --launchdarkly /tmp/launchdarkly --sdds3 ${VUT_WAREHOUSE_ROOT}  shell=true
    ${result}=  Run Process    ls -lR ${VUT_WAREHOUSE_ROOT}     shell=True
    Log  ${result.stdout}
    Set Suite Variable    ${GL_handle}    ${handle}

EDR Suite Teardown
    Upgrade Resources Suite Teardown
    Stop Local SDDS3 Server
    Remove Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL

Setup SUS all develop
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly


Setup SUS all 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly

Setup SUS only edr 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly

Setup SUS only base 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly

Setup SUS all non-base plugins 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly

EDR Test Setup
    Upgrade Resources Test Setup

Report Audit Link Ownership
    ${result} =  Run Process   auditctl -s   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${result} =  Run Process   auditctl -s | grep pid | awk '{print $2}'  shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${owner}=  Run Process  ps -ef | grep ${result.stdout}  shell=True
    log  ${owner.stdout}
    log  ${owner.stderr}
    [Return]  ${owner.stdout}


EDR Test Teardown
    Run Keyword if Test Failed    Log Status Of Auditd
    Run Keyword if Test Failed    Report Audit Link Ownership
    Run Keyword if Test Failed    Report On MCS_CA
    Run Keyword if Test Failed    Log File  ${UPDATE_CONFIG}

    Upgrade Resources Test Teardown   UninstallAudit=False

EDR Uninstall Teardown
    Require Watchdog Running
    Upgrade Resources Test Teardown

Uninstall EDR Plugin
    ${result} =  Run Process     ${EDR_PLUGIN_PATH}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

Wait For EDR Status
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml

Install EDR Directly
    ${EDR_SDDS_DIR} =  Get SSPL EDR Plugin SDDS
    ${result} =    Run Process  bash -x ${EDR_SDDS_DIR}/install.sh 2> /tmp/edr_install.log   shell=True
    ${stderr} =   Get File  /tmp/edr_install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr} ${stderr}"
    Log  ${result.stdout}
    Log  ${stderr}
    Log  ${result.stderr}
    Wait For EDR to be Installed

Check Osquery Executable Not Running
    ${result} =    Run Process  pgrep  -a  osquery
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Kill OSQuery
    ${result} =  Run Process  pgrep osquery | xargs kill -9  shell=true
    Should Be Equal As Strings  ${result.rc}  0
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  Check Osquery Executable Not Running
    Dump All Sophos Processes

Kill OSQuery And Wait Until Osquery Running Again
    #TODO LINUXDAR-3974 : timeouts in this keyword are very long to smooth flakiness out of tests.
    # When/if a product change is made please reduce them to more reasonable lengths.
    Kill OSQuery
    # We expect edr to restart osquery ~10s after noticing it's died, but osquery 5.0.1 can trigger backoffs on launching the osquery worker
    # process which throws our timing off. the product does reliably stabilise though.
    Wait Until EDR OSQuery Running  40
    # osquery 5.0.1 is behaving differently and can take longer to stabilize when restarting quickly, this sleep forces
    # the test to give it time to stabilize before continuing
    sleep  10

Check EDR Osquery Executable Running
    #Check both osquery instances are running
    ${result} =    Run Process  pgrep -a osquery | grep plugins/edr | wc -l  shell=true
    Should Be Equal As Integers    ${result.stdout}    2       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check EDR Osquery Executable Not Running
    ${result} =    Run Process  pgrep  -a  osquery | grep plugins/edr
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Wait Until EDR OSQuery Running
    [Arguments]  ${WaitInSecs}=30
    Wait Until Keyword Succeeds
    ...  ${WaitInSecs} secs
    ...  1 secs
    ...  Check EDR Osquery Executable Running

Check EDR Plugin Uninstalled
    EDR Plugin Is Not Running
    Check EDR Osquery Executable Not Running
    Should Not Exist  ${EDR_DIR}

Wait Until EDR Uninstalled
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  Check EDR Plugin Uninstalled

Get Edr OsQuery PID
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   10 secs
    ...   Check EDR Osquery Executable Running
    ${edr_osquery_pid} =    Run Process  pgrep -a osquery | grep plugins/edr | grep -v osquery.conf | head -n1 | cut -d " " -f1  shell=true
    [return]  ${edr_osquery_pid.stdout}

Log Status Of Auditd
    ${result} =  Run Process  systemctl  status  auditd
    Log  ${result.stdout}
    Log  ${result.stderr}

Create Query Packs
    ${pack_content} =  Set Variable   {"query": "select * from uptime;","interval": 100, "denylist": false}
    @{QUERY_PACK_DIRS} =    Create List    ${QUERY_PACKS_PATH}    ${OSQUERY_CONF_PATH}
    FOR    ${dir}    IN    @{QUERY_PACK_DIRS}
        Create File   ${dir}/sophos-scheduled-query-pack.conf   {"schedule": {"latest_xdr_query": ${pack_content}}}
        Create File   ${dir}/sophos-scheduled-query-pack.mtr.conf   {"schedule": {"latest_mtr_query": ${pack_content}}}
    END

Cleanup Query Packs
    @{QUERY_PACK_DIRS} =    Create List    ${QUERY_PACKS_PATH}    ${OSQUERY_CONF_PATH}
    FOR    ${dir}    IN    @{QUERY_PACK_DIRS}
        Remove File   ${dir}/sophos-scheduled-query-pack.conf
        Remove File   ${dir}/sophos-scheduled-query-pack.mtr.conf
    END