*** Settings ***
Library     Process
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot
*** Variables ***
${AV_PLUGIN_PATH}                   ${SOPHOS_INSTALL}/plugins/av
${AV_LOG_PATH}                      ${AV_PLUGIN_PATH}/log/
${AV_LOG_FILE}                      ${AV_PLUGIN_PATH}/log/av.log
${THREAT_DETECTOR_LOG_PATH}         ${AV_PLUGIN_PATH}/chroot/log/sophos_threat_detector.log
${SOPHOS_THREAT_DETECTOR_BINARY}    ${AV_PLUGIN_PATH}/sbin/sophos_threat_detector
${CLS_PATH}                         ${AV_PLUGIN_PATH}/bin/avscanner
${PLUGIN_BINARY}                    ${AV_PLUGIN_PATH}/sbin/av
${SULDownloaderLog}                 ${SOPHOS_INSTALL}/logs/base/suldownloader.log

${VIRUS_DETECTED_RESULT}            ${24}
${CLEAN_STRING}                     I am not a virus
${EICAR_STRING}                     X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*

*** Keywords ***
Install AV Plugin Directly
    ${AV_SDDS_DIR} =  Get SSPL Anti Virus Plugin SDDS
    ${result} =    Run Process  bash -x ${AV_SDDS_DIR}/install.sh 2> /tmp/install.log   shell=True
    ${error} =  Get File  /tmp/install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}, ${error}"
    Log  ${error}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Check AV Plugin Installed

Check AV Plugin Installed
    Check Log Does Not Contain  Failed to install as setcap is not installed  ${SULDownloaderLog}  SulDownloaderLog
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
    Run Shell Process  pidof ${PLUGIN_BINARY}   OnError=AV not running

Check AV Plugin Executable Not Running
    ${result} =    Run Process  pgrep  -a  ${PLUGIN_BINARY}
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Stop AV Plugin
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  av

Start AV Plugin
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  av

AV Plugin Log Contains
    [Arguments]  ${TextToFind}
    File Should Exist  ${AV_LOG_FILE}
    ${fileContent}=  Get File  ${AV_LOG_FILE}
    Should Contain  ${fileContent}    ${TextToFind}

Threat Detector Log Contains
    [Arguments]  ${input}
    ${fileContent}=  Get File  ${THREAT_DETECTOR_LOG_PATH}
    Should Contain  ${fileContent}    ${input}

Wait until threat detector running
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Threat Detector Log Contains  SophosThreatDetectorImpl <> Starting USR1 monitor

Check AV Plugin Permissions
    ${rc}   ${output} =    Run And Return Rc And Output   find ${AV_PLUGIN_PATH} -user sophos-spl-user -print
    Should Be Equal As Integers  ${rc}  0
    Should Be Empty  ${output}

Check AV Plugin Can Scan Files
    Create File     /tmp/clean_file    ${CLEAN_STRING}
    Create File     /tmp/dirty_file    ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLS_PATH} /tmp/clean_file /tmp/dirty_file
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
