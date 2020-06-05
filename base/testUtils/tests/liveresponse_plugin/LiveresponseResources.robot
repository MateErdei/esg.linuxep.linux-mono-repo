*** Settings ***
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Resource  ../upgrade_product/UpgradeResources.robot


*** Variables ***
${Thumbprint}               2D03C43BFC9EBE133E0C22DF61B840F94B3A3BD5B05D1D715CC92B2DEBCB6F9D
${websocket_server_url}     wss://localhost.com/end_to_end_test



*** Keywords ***
Wait For Liveresponse to be Installed
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   10 secs
    ...   File Should exist    ${SOPHOS_INSTALL}/plugins/liveresponse/bin/liveresponse

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   Liveresponse Plugin Is Running



Liveresponse Plugin Is Running
    ${result} =    Run Process  pgrep  liveresponse
    Should Be Equal As Integers    ${result.rc}    0

Liveresponse Plugin Is Not Running
    ${result} =    Run Process  pgrep  liveresponse
    Should Not Be Equal As Integers    ${result.rc}    0   liveresponse PLugin still running

Restart Liveresponse Plugin
    [Arguments]  ${clearLog}=False
    Wdctl Stop Plugin  liveresponse
    Run Keyword If   ${clearLog}   Remove File  ${LIVERESPONSE_DIR}/log/liveresponse.log
    Wdctl Start Plugin  liveresponse

Start Liveresponse Session And Wait For Agent To Make A Connection
    [Arguments]  ${query}
    Send Liveresponse Command From Fake Cloud    ProcessEvents  ${query}  command_id=queryname
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Server Connection


Check Liveresponse Agent Connection Success

#    ${EDR_LOG_CONTENT}=  Get File  ${LI}/log/edr.log
#    Should Contain  ${EDR_LOG_CONTENT}   EDR configuration set to disable AuditD
#    Should Contain  ${EDR_LOG_CONTENT}   Successfully stopped service: auditd
#    Should Contain  ${EDR_LOG_CONTENT}   Successfully disabled service: auditd

Check Liveresponse Log Contains
    [Arguments]  ${string_to_contain}
    ${LR_LOG_CONTENT}=  Get File  ${LIVERESPONSE_DIR}/log/liveresponse.log
    Should Contain  ${LR_LOG_CONTENT}   ${string_to_contain}

Wait Keyword Succeed
    [Arguments]  ${keyword}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  ${keyword}


Uninstall Liveresponse Plugin
    ${result} =  Run Process     ${LIVERESPONSE_DIR}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

Wait For Liveresponse Status
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/status/liveresponse_status.xml

Install Liveresponse Directly
    ${LR_SDDS_DIR} =  Get SSPL Liveresponse Plugin SDDS
    ${result} =    Run Process  bash -x ${LR_SDDS_DIR}/install.sh   shell=True
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}"
    Log  ${result.stdout}
    Log  ${result.stderr}
    Wait For Liveresponse to be Installed

Check Liveresponse Agent Executable is Running
    [Arguments]  ${num_sessions}=1
    ${result} =    Run Process  pgrep -a sophos-live-terminal | grep plugins/liveresponse | wc -l  shell=true
    Should Be Equal As Integers    ${result.stdout}    ${num_sessions}       msg="stdout:${result.stdout}\n err: ${result.stderr}"

Check Liveresponse Agent Executable is Not Running
    ${result} =    Run Process  pgrep -a sophos-live-terminal | grep plugins/liveresponse
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\n err: ${result.stderr}"

Check Liveresponse Plugin Uninstalled
    Liveresponse Plugin Is Not Running
    Check Liveresponse Agent Executable is Not Running
    Should Not Exist  ${LIVERESPONSE_DIR}

Wait Until Liveresponse Uninstalled
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  Check EDR Plugin Uninstalled