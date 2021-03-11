*** Settings ***
Library     Process
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot
*** Variables ***
${AV_PLUGIN_PATH}                   ${SOPHOS_INSTALL}/plugins/av
${AV_LOG_FILE}                      ${AV_PLUGIN_PATH}/log/av.log

*** Keywords ***
Check AV Plugin Installed
    File Should Exist   ${AVPLUGIN_PATH}/bin/avscanner
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check AV Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  AV Plugin Log Contains  av <> Starting scanScheduler

Check AV Plugin Running
    ${result} =    Run Process  pgrep  av
    Should Be Equal As Integers    ${result.rc}    0

AV Plugin Log Contains
    [Arguments]  ${TextToFind}
    File Should Exist  ${AV_LOG_FILE}
    ${fileContent}=  Get File  ${AV_LOG_FILE}
    Should Contain  ${fileContent}    ${TextToFind}
