*** Settings ***
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Resource  ../upgrade_product/UpgradeResources.robot


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

*** Keywords ***

Wait For EDR to be Installed
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   10 secs
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
    ...   5 secs
    ...   File Should exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/options.conf

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
    Wdctl Stop Plugin  edr
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   EDR Plugin Is Not Running
    Run Keyword If   ${clearLog}   Remove File  ${EDR_DIR}/log/edr.log
    Run Keyword If   ${installQueryPacks}   Create Query Packs
    Wdctl Start Plugin  edr
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   EDR Plugin Is Running

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
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    #TODO: LINUXDAR-4015 remove once issue is closed
    ${result} =  Run Process    free
    Log  ${result.stdout}

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

Check AuditD Executable Running
    ${result} =    Run Process  pgrep  ^auditd
    Should Be Equal As Integers    ${result.rc}    0       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check AuditD Executable Not Running
    ${result} =    Run Process  pgrep  ^auditd
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check AuditD Service Disabled
    ${result} =    Run Process  systemctl  is-enabled  auditd
    log  ${result.stdout}
    log  ${result.stderr}
    log  ${result.rc}
    Should Not Be Equal As Integers    ${result.rc}    0

Check EDR Log Shows AuditD Has Been Disabled
    ${EDR_LOG_CONTENT}=  Get File  ${EDR_DIR}/log/edr.log
    Should Contain  ${EDR_LOG_CONTENT}   EDR configuration set to disable AuditD
    Should Contain  ${EDR_LOG_CONTENT}   Successfully stopped service: auditd
    Should Contain  ${EDR_LOG_CONTENT}   Successfully disabled service: auditd
    Should Not Contain  ${EDR_LOG_CONTENT}   Failed to mask journald audit socket

Check EDR Log Shows AuditD Has Not Been Disabled
    ${EDR_LOG_CONTENT}=  Get File  ${EDR_DIR}/log/edr.log
    Should Not Contain  ${EDR_LOG_CONTENT}   Successfully disabled service: auditd
    Should Contain  ${EDR_LOG_CONTENT}   EDR configuration set to not disable AuditD
    Should Contain  ${EDR_LOG_CONTENT}   AuditD is running, it will not be possible to obtain event data.

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

Wait Keyword Succeed
    [Arguments]  ${keyword}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  ${keyword}

Install And Enable AuditD If Required
    ${Result}=  Is Ubuntu
    Run Keyword If   ${Result}==${True}
    ...   Run Shell Process   apt-get install auditd -y    OnError=failed to install auditd  timeout=200s

    #  Assume all other supported platforms install software using YUM
    IF  ${Result}!=${True}
      Run Shell Process   yum install audit -y    OnError=failed to install auditd  timeout=200s
    END

    Run Shell Process   systemctl start auditd    OnError=failed to start auditd

Uninstall AuditD If Required
    ${Result}=  Is Ubuntu
    Run Keyword If   ${Result}==${True}
    ...   Run Shell Process   apt-get remove auditd -y    OnError=failed to remove auditd  timeout=200s

    #  Assume all other supported platforms install software using YUM
    IF  ${Result}!=${True}
      Run Shell Process   yum remove audit -y    OnError=failed to remove auditd  timeout=200s
    END

EDR Suite Setup
    UpgradeResources.Suite Setup
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Set Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL  1
    # LINUXDAR-7059: On SUSE the thin installer fails to connect to the first SDDS3 server so workaround for now by running twice
    ${result} =  Run Process    bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly /tmp/launchdarkly --sdds3 ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/repo  shell=true    timeout=10s
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly /tmp/launchdarkly --sdds3 ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/repo  shell=true
    Set Suite Variable    ${GL_handle}    ${handle}

EDR Suite Teardown
    UpgradeResources.Suite Teardown
    Stop Local SDDS3 Server
    Remove Environment Variable  SOPHOS_CORE_DUMP_ON_PLUGIN_KILL

Setup SUS all develop
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-MDR.json  /tmp/launchdarkly


Setup SUS all 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-MDR.json  /tmp/launchdarkly

Setup SUS only edr 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-MDR.json  /tmp/launchdarkly

Setup SUS only base 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-MDR.json  /tmp/launchdarkly

Setup SUS all non-base plugins 999
    Remove Directory   /tmp/launchdarkly   recursive=True
    Create Directory   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly/release.linuxep.ServerProtectionLinux-Base.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-AV.json   /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  /tmp/launchdarkly
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-MDR.json  /tmp/launchdarkly

EDR Test Setup
    UpgradeResources.Test Setup

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

    UpgradeResources.Test Teardown   UninstallAudit=False

EDR Uninstall Teardown
    Require Watchdog Running
    Test Teardown

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

Check EDR Osquery Executable Running
    #Check both osquery instances are running
    ${result} =    Run Process  pgrep -a osquery | grep plugins/edr | wc -l  shell=true
    Should Be Equal As Integers    ${result.stdout}    2       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check EDR Osquery Executable Not Running
    ${result} =    Run Process  pgrep  -a  osquery | grep plugins/edr
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

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
        Create File   ${dir}/sophos-scheduled-query-pack-next.conf    {"schedule": {"next_xdr_query": ${pack_content}}}
        Create File   ${dir}/sophos-scheduled-query-pack-next.mtr.conf    {"schedule": {"next_mtr_query": ${pack_content}}}
    END

Cleanup Query Packs
    @{QUERY_PACK_DIRS} =    Create List    ${QUERY_PACKS_PATH}    ${OSQUERY_CONF_PATH}
    FOR    ${dir}    IN    @{QUERY_PACK_DIRS}
        Remove File   ${dir}/sophos-scheduled-query-pack.conf
        Remove File   ${dir}/sophos-scheduled-query-pack.mtr.conf
        Remove File   ${dir}/sophos-scheduled-query-pack-next.conf
        Remove File   ${dir}/sophos-scheduled-query-pack-next.mtr.conf
    END