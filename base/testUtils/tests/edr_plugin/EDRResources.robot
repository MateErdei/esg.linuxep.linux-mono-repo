*** Settings ***
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Resource  ../upgrade_product/UpgradeResources.robot


*** Variables ***
#EDR Telemetry variables
${COMPONENT_TEMP_DIR}  /tmp/edr_component
${CRASH_QUERY} =  WITH RECURSIVE counting (curr, next) AS ( SELECT 1,1 UNION ALL SELECT next, curr+1 FROM counting LIMIT 10000000000 ) SELECT group_concat(curr) FROM counting;
${SOPHOS_INFO_QUERY} =  SELECT endpoint_id from sophos_endpoint_info;
${SIMPLE_QUERY_1_ROW} =  SELECT * from users limit 1;
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



EDR Plugin Is Running
    ${result} =    Run Process  pgrep  edr
    Should Be Equal As Integers    ${result.rc}    0

EDR Plugin Is Not Running
    ${result} =    Run Process  pgrep  edr
    Should Not Be Equal As Integers    ${result.rc}    0   EDR PLugin still running

Restart EDR Plugin
    [Arguments]  ${clearLog}=False
    Wdctl Stop Plugin  edr
    Run Keyword If   ${clearLog}   Remove File  ${EDR_DIR}/log/edr.log
    Wdctl Start Plugin  edr

Install EDR
    [Arguments]  ${policy}  ${args}=${None}
    Start Local Cloud Server  --initial-alc-policy  ${policy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${policy}  args=${args}

    Send ALC Policy And Prepare For Upgrade  ${policy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check Suldownloader Log Contains In Order     Installing product: ServerProtectionLinux-Plugin-EDR   Generating the report file

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
    Run Keyword Unless  ${Result}==${True}
    ...  Run Shell Process   yum install audit -y    OnError=failed to install auditd  timeout=200s

    Run Shell Process   systemctl start auditd    OnError=failed to start auditd

Uninstall AuditD If Required
    ${Result}=  Is Ubuntu
    Run Keyword If   ${Result}==${True}
    ...   Run Shell Process   apt-get remove auditd -y    OnError=failed to remove auditd  timeout=200s

    #  Assume all other supported platforms install software using YUM
    Run Keyword Unless  ${Result}==${True}
    ...  Run Shell Process   yum remove audit -y    OnError=failed to remove auditd  timeout=200s

EDR Suite Setup
    UpgradeResources.Suite Setup
    ${result} =  Run Process  curl -v https://ostia.eng.sophos/latest/sspl-warehouse/master   shell=True
    Should Be Equal As Integers  ${result.rc}  0  "Failed to Verify connection to Update Server. Please, check endpoint is configured. (Hint: tools/setup_sspl/setupEnvironment2.sh).\nStdOut: ${result.stdout}\nStdErr: ${result.stderr}"

EDR Suite Teardown
    UpgradeResources.Suite Teardown

EDR Test Setup
    UpgradeResources.Test Setup

Report Audit Link Ownership
    ${result} =  Run Process   auditctl -s   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${result} =  Run Process   auditctl -s | grep pid | awk '{print $2}'  shell=True
    ${owner}=  Run Process  ps -ef | grep ${result.stdout}  shell=True
    log  ${owner.stdout}
    log  ${owner.stderr}


EDR Test Teardown
    Run Keyword if Test Failed    Report Audit Link Ownership
    Run Keyword if Test Failed    Report On MCS_CA
    Run Keyword if Test Failed    Log File  ${SOPHOS_INSTALL}/base/update/var/update_config.json

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
    ${edr_osquery_pid} =    Run Process  pgrep -a osquery | grep plugins/edr | grep -v osquery.conf | head -n1 | cut -d " " -f1  shell=true
    [return]  ${edr_osquery_pid.stdout}