*** Settings ***
Library     Process
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot
*** Variables ***
${AV_PLUGIN_PATH}                   ${SOPHOS_INSTALL}/plugins/av
${AV_LOG_PATH}                      ${AV_PLUGIN_PATH}/log/
${AV_LOG_FILE}                      ${AV_PLUGIN_PATH}/log/av.log
${THREAT_DETECTOR_LOG_PATH}         ${AV_PLUGIN_PATH}/log/sophos_threat_detector.log
${SOPHOS_THREAT_DETECTOR_BINARY}    ${AV_PLUGIN_PATH}/sbin/sophos_threat_detector

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

Threat Detector Log Contains
    [Arguments]  ${input}
    ${fileContent}=  Get File  ${THREAT_DETECTOR_LOG_PATH}
    Should Contain  ${fileContent}    ${input}

Check Sophos Threat Detector Running
    Run Shell Process  pidof ${SOPHOS_THREAT_DETECTOR_BINARY}   OnError=sophos_threat_detector not running

Wait until threat detector running
    # wait for AV Plugin to initialize
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check Sophos Threat Detector Running
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  2 secs
    ...  Threat Detector Log Contains  UnixSocket <> Starting listening on socket