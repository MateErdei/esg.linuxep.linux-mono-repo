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