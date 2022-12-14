*** Settings ***
Library     Process
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/DownloadAVSupplements.py

Resource  ../GeneralTeardownResource.robot
*** Variables ***
${AV_PLUGIN_PATH}                   ${SOPHOS_INSTALL}/plugins/av
${AV_LOG_PATH}                      ${AV_PLUGIN_PATH}/log/
${AV_LOG_FILE}                      ${AV_PLUGIN_PATH}/log/av.log
${THREAT_DETECTOR_LOG_PATH}         ${AV_PLUGIN_PATH}/chroot/log/sophos_threat_detector.log
${THREAT_REPORT_SOCKET_PATH}        ${AV_PLUGIN_PATH}/chroot/var/threat_report_socket
${SOPHOS_THREAT_DETECTOR_BINARY}    ${AV_PLUGIN_PATH}/sbin/sophos_threat_detector
${CLS_PATH}                         ${AV_PLUGIN_PATH}/bin/avscanner
${PLUGIN_BINARY}                    ${AV_PLUGIN_PATH}/sbin/av
${SULDownloaderLog}                 ${SOPHOS_INSTALL}/logs/base/suldownloader.log

${VIRUS_DETECTED_RESULT}            ${24}
${CLEAN_STRING}                     I am not a virus
${EICAR_STRING}                     X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*

*** Keywords ***
Install AV Plugin Directly
    ${AV_SDDS_DIR} =  Setup Av Install
    ${result}=  Run Process  bash  -x  ${AV_SDDS_DIR}/install.sh  stdout=/tmp/av_install.log  stderr=STDOUT  timeout=120s
    Should Be Equal As Integers    ${result.rc}    0   "AV Installer failed"
    Log  ${result.stderr}
    Log  ${result.stdout}
    Check AV Plugin Installed Directly

Install Virus data
    Get Av Supplements

Check AV Plugin Installed
    Check Log Does Not Contain  Failed to install as setcap is not installed  ${SULDownloaderLog}  SulDownloaderLog
    Check AV Plugin Installed Directly

Check AV Plugin Installed Directly
    File Should Exist   ${AVPLUGIN_PATH}/bin/avscanner
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check AV Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  AV Plugin Log Contains  <> Starting scanScheduler

Check AV Plugin Running
    ${result} =    Run Process  pgrep  -f  ${PLUGIN_BINARY}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0

Check AV Plugin Executable Not Running
    ${result} =    Run Process  pgrep  -f  ${PLUGIN_BINARY}
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Stop AV Plugin
    ${result} =    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  av
    Should Be Equal As Integers    ${result.rc}    0

Start AV Plugin
    ${result} =    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  av
    Should Be Equal As Integers    ${result.rc}    0

Stop Threat Detector
    ${result} =    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  threat_detector
    Should Be Equal As Integers    ${result.rc}    0

Start Threat Detector
    ${result} =    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  threat_detector
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
    [Arguments]  ${dirty_file}=/tmp/dirty_file
    Create File     /tmp/clean_file    ${CLEAN_STRING}
    Create File     ${dirty_file}    ${EICAR_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLS_PATH} /tmp/clean_file ${dirty_file}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
