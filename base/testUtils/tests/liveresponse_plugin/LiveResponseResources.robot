*** Settings ***
Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${LIBS_DIRECTORY}/WebsocketWrapper.py

*** Variables ***
${LIVE_RESPONSE_PLUGIN_PATH}                  ${SOPHOS_INSTALL}/plugins/liveresponse
${LIVE_RESPONSE_LOG_FILE} =                   ${LIVE_RESPONSE_PLUGIN_PATH}/log/liveresponse.log

*** Keywords ***
Install Live Response Directly
    ${LIVE_RESPONSE_SDDS_DIR} =  Get SSPL Live Response Plugin SDDS
    ${result} =    Run Process  bash -x ${LIVE_RESPONSE_SDDS_DIR}/install.sh   shell=True
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}"
    Log  ${result.stdout}
    Log  ${result.stderr}
    Check Live Response Plugin Installed

Check Live Response Plugin Installed
    File Should Exist   ${LIVE_RESPONSE_PLUGIN_PATH}/bin/sophos-live-terminal
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Live Response Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Live Response Plugin Log Contains  liveresponse <> Entering the main loop

Check Live Response Plugin Running
    ${result} =    Run Process  pgrep  liveresponse
    Should Be Equal As Integers    ${result.rc}    0

Live Response Plugin Log Contains
    [Arguments]  ${TextToFind}
    File Should Exist  ${LIVE_RESPONSE_LOG_FILE}
    ${fileContent}=  Get File  ${LIVE_RESPONSE_LOG_FILE}
    Should Contain  ${fileContent}    ${TextToFind}

Check Liveresponse Agent Executable is Not Running
    ${result} =    Run Process  pgrep  -a  sophos-live-terminal
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Restart Liveresponse Plugin
    [Arguments]  ${clearLog}=False
    Wdctl Stop Plugin  liveresponse
    Run Keyword If   ${clearLog}   Remove File  ${LIVERESPONSE_DIR}/log/liveresponse.log
    Wdctl Start Plugin  liveresponse

Setup Suite Tmp Dir
    [Arguments]    ${directory_path}
    Set Suite Variable   ${SUITE_TMP_DIR}   ${directory_path}
    Remove Directory    ${SUITE_TMP_DIR}   recursive=True
    Create Directory    ${SUITE_TMP_DIR}

Liveresponse Test Setup
    Require Installed
    Start Websocket Server
    Restart Liveresponse Plugin  True
    Check Live Response Plugin Installed
    Empty Directory  ${MCS_DIR}/action/
    Empty Directory  ${LIVE_RESPONSE_PLUGIN_PATH}/var/

Liveresponse Test Teardown
    Stop Websocket Server
    ${files} =  List Directory   ${MCS_DIR}/action/
    ${liveterminal_server_log} =  Liveterminal Server Log File
    Log File  ${liveterminal_server_log}
    General Test Teardown

Liveresponse Suite Setup
    Generate Local Fake Cloud Certificates
    Setup Suite Tmp Dir   ./tmp
    Setup Base FakeCloud And FakeCentral-LT Servers
    Install Live Response Directly

Liveresponse Suite Teardown
    Shutdown MCS Push Server
    Stop Proxy Servers
    Remove Environment Variable    https_proxy
    Stop Local Cloud Server
    Uninstall SSPL
    uninstall LT Server Certificates

Setup Base FakeCloud And FakeCentral-LT Servers
    Install LT Server Certificates
    Start MCS Push Server
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Set Local CA Environment Variable

    Require Fresh Install
    create file  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Local Cloud Server
    Override LogConf File as Global Level  DEBUG
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log   liveresponse   DEBUG