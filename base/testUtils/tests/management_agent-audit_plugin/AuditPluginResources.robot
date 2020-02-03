*** Settings ***
Library     ${libs_directory}/AuditSupport.py
Library     ${libs_directory}/CapnSubscriber.py
Resource  ../watchdog/LogControlResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot


*** Variable ***
${SSPL_AUDIT_RIGID_NAME}   ServerProtectionLinux-Plugin-AuditPlugin
${SSPL_AUDIT_BINARY_NAME}  sophosauditplugin

*** Keywords ***

Get SSPL Audit Full Path
    [Return]  ${SOPHOS_INSTALL}/plugins/AuditPlugin/bin/${SSPL_AUDIT_BINARY_NAME}

Get SSPL Audit Log File Path
    [Return]  ${SOPHOS_INSTALL}/plugins/AuditPlugin/log/AuditPlugin.log

Install SSPL Audit Plugin From
    [Arguments]  ${sspl_audit_plugin_sdds}
    Set Log Level For Component And Reset and Return Previous Log  AuditPlugin  DEBUG
    Install Package Via Warehouse  ${sspl_audit_plugin_sdds}    ${SSPL_AUDIT_RIGID_NAME}

Check SSPL Audit Plugin Running
    ## SSPL_AUDIT_BINARY_NAME > 15 chars, so need to use -f
    ${result} =    Run Process  pgrep  -f  ${SSPL_AUDIT_BINARY_NAME}
    Should Be Equal As Integers    ${result.rc}    0   msg=Process ${SSPL_AUDIT_BINARY_NAME} is not running

Uninstall And Terminate All Processes
    Uninstall SSPL
    Terminate All Processes  kill=True

Management Agent Log Contains
    [Arguments]  ${TextToFind}
    ${management_agent_log} =  Get File  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Should Contain  ${management_agent_log}  ${TextToFind}

Audit Log Contains
    [Arguments]  ${TextToFind}
    ${AUDIT_LOG_FILE}=   Get SSPL Audit Log File Path
    File Should Exist  ${AUDIT_LOG_FILE}
    ${fileContent}=  Get File  ${AUDIT_LOG_FILE}
    Should Contain  ${fileContent}    ${TextToFind}


Check AuditPlugin Installed
    ${SSPL_AUDIT_FULL_PATH}=  Get SSPL Audit Full Path
    File Should Exist   ${SSPL_AUDIT_FULL_PATH}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check SSPL Audit Plugin Running
    Wait Until Keyword Succeeds
    ...  6 secs
    ...  1 secs
    ...  Audit Log Contains  Audit Plugin setup


Ensure A Clean Start
    Run Keyword and Ignore Error   Remove File  ./tmp/fake_multi_subscriber.log
    Run Keyword and Ignore Error   Remove File  ./tmp/CapnSubscriber.log
    Disable System Auditd
    Clear Auditd Logs

Setup SSPL Audit Plugin Tmpdir
    Set Test Variable  ${tmpdir}            /tmp/SSPL-Audit-Plugin-Tests
    Remove Directory   ${tmpdir}    recursive=True
    Create Directory   ${tmpdir}
    Run Process  chown  sophos-spl-user:sophos-spl-group  ${tmpdir}
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${tmpdir}


Default Test Teardown
    Clear Subscriber Stop
    General Test Teardown
    Run Keyword If Test Failed  Dump all mcs events
    Run Keyword If Test Failed  Dump All Processes
    Run Keyword If Test Failed  Display Auditd Logs
    Run Keyword If Test Failed  Run Keyword and Ignore Error  Display SulDownloaderOutput
    Run Keyword If Test Failed  Log Status Of Audit Daemon
    Clean Up Warehouse Temp Dir
    Remove Directory   ./tmp        recursive=True  #Pub/Sub logging is done here so needs cleaning up

Log Audit Plugin Log and Remove it
    ${AUDIT_LOG_FILE}=   Get SSPL Audit Log File Path
    Run Keyword and Ignore Error  Log File  ${AUDIT_LOG_FILE}
    Run Keyword and Ignore Error  Remove File  ${AUDIT_LOG_FILE}

Display SulDownloaderOutput
    Log File  ${tmpdir}/output.json
    Log File  ${tmpdir}/update_config.json


Install Audit Plugin Directly
    ${SDDS} =  Get SSPL Audit Plugin SDDS
    Set Log Level For Component And Reset and Return Previous Log  AuditPlugin  DEBUG
    ${result} =    Run Process  bash  -x  ${SDDS}/install.sh
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Check AuditPlugin Installed

Uninstall Audit Plugin
    File Should Exist  ${SOPHOS_INSTALL}/plugins/AuditPlugin/bin/sophosauditplugin
    ${result} =    Run Process  ${SOPHOS_INSTALL}/plugins/AuditPlugin/bin/uninstall.sh
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}

Install Audit Plugin From Warehouse
    ${SDDS} =  Copy Sspl Audit Plugin Sdds
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install SSPL Audit Plugin From  ${SDDS}
    Check AuditPlugin Installed

Run AuditPlugin Against Example
    [Arguments]  ${NameOfTheExample}  ${distribution}=ubuntu
    ${PathOfFile} =   Catenate  SEPARATOR=  ${SUPPORT_FILES}/auditd_data/  ${distribution}  /  ${NameOfTheExample}  AuditEvents.bin
    ${PathOfAuditPlugin} =   Catenate  SEPARATOR=  ${SOPHOS_INSTALL}/plugins/AuditPlugin/bin/${SSPL_AUDIT_BINARY_NAME}
    Should Be Equal  ${PathOfAuditPlugin}  /opt/sophos-spl/plugins/AuditPlugin/bin/${SSPL_AUDIT_BINARY_NAME}
    File Should Exist  ${PathOfAuditPlugin}
    ${handle} =   Start Process   ${SUPPORT_FILES}/DelayStartFileReadAndDelayCloseStdIn.sh  ${PathOfFile}  |  ${PathOfAuditPlugin}   shell=True
    ${result} =   Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}

Setup and Run Audit Plugin Against Example
    [Arguments]  ${NameOfTheExample}  ${distribution}=ubuntu
    Install Audit Plugin Directly
    Disable System Auditd
    Start Subscriber
    Log Audit Plugin Log and Remove it
    Sleep  2 secs
    Run AuditPlugin Against Example  ${NameOfTheExample}  ${distribution}
    ${messages} =   Subscriber Get All Messages
    [return]  ${messages}


Kill Audit Plugin
    ${SSPL_AUDIT_FULL_PATH}=  Get SSPL Audit Full Path
    Kill By File and Wait it to Shutdown  ${SSPL_AUDIT_FULL_PATH}   secs2wait=3  number_of_attempts=5

Log Status Of Audit Daemon
    ${result} =  Run Process    systemctl  status  auditd
    Log  ${result.stdout}

