*** Settings ***

Test Setup       EDR Test Setup
Test Teardown    EDR Test Teardown

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py

Resource    ../mdr_plugin/MDRResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    ../watchdog/LogControlResources.robot
Resource    EDRResources.robot
Resource    ../mcs_router/McsPushClientResources.robot
Resource   ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../liveresponse_plugin/LiveResponseResources.robot

Default Tags   EDR_PLUGIN   FAKE_CLOUD   THIN_INSTALLER  INSTALLER
Force Tags  LOAD1


*** Variables ***
${EDR_STATUS_XML}                   ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
${CACHED_STATUS_XML} =              ${SOPHOS_INSTALL}/base/mcs/status/cache/LiveQuery.xml
${SULDOWNLOADER_LOG_PATH}           ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${SULDownloaderLogDowngrade}        ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${WDCTL_LOG_PATH}                   ${SOPHOS_INSTALL}/logs/base/wdctl.log

*** Test Cases ***
EDR Uninstaller Does Not Report That It Could Not Remove EDR If Watchdog Is Not Running
    [Teardown]  EDR Uninstall Teardown
    Run Full Installer
    Install EDR Directly
    Wait Until EDR OSQuery Running
    ${systemctlResult} =  Run Process   systemctl stop sophos-spl   shell=yes
    Check Watchdog Not Running
    Should Be Equal As Strings  ${systemctlResult.rc}  0

    ${result} =  Uninstall EDR Plugin
    Should Not Contain  ${result.stderr}  Failed to remove edr: Watchdog is not running
EDR sets up syslog pipe correctly
    [Tags]  EXCLUDE_UBUNTU18  EDR_PLUGIN   FAKE_CLOUD   THIN_INSTALLER  INSTALLER
    Run Full Installer
    Install EDR Directly

    ${result}=  Run Process  cat   /etc/selinux/targeted/contexts/files/file_contexts.local
    LOG  ${result.stdout}
    LOG  ${result.stderr}
    ${result}=  Run Process  ls   -Z  ${SOPHOS_INSTALL}/shared/syslog_pipe
    should contain  ${result.stdout}   var_log_t

EDR Removes Ipc And Status Files When Uninstalled
    Run Full Installer
    Install EDR Directly
    Wait Until EDR OSQuery Running
    Stop Management Agent Via WDCTL
    Start Management Agent Via WDCTL
    Wait for EDR Status

    # ${IPC_FILE} is a socket, and therefore is not picked up by "File Should Exist"
    Should Exist            ${IPC_FILE}
    File Should Exist       ${EDR_STATUS_XML}
    File Should Exist       ${CACHED_STATUS_XML}


    Uninstall EDR Plugin

    # Similarly, "File Should Not Exist" will always pass on ${IPC_FILE}
    Should Not Exist        ${IPC_FILE}
    File Should Not Exist   ${EDR_STATUS_XML}
    File Should Not Exist   ${CACHED_STATUS_XML}

EDR Handles Live Query
    Start Local Cloud Server
    Regenerate Certificates
    Set Local CA Environment Variable

    Run Full Installer
    Install EDR Directly
    Wait Until EDR OSQuery Running

    Wdctl Stop Plugin  edr
    Override LogConf File as Global Level  DEBUG
    Mark Edr Log
    Wdctl Start Plugin  edr
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked EDR Log Contains  Plugin preparation complete

    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Mark Edr Log
    Register With Fake Cloud
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked EDR Log Contains  Applying new policy with APPID: LiveQuery
    Mark Edr Log
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  3 secs
    ...  Check Marked EDR Log Contains  SophosExtension running

    Send Query From Fake Cloud  Test Query Special  select name from processes  command_id=firstcommand
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains  /commands/applications/MCS;ALC;LiveQuery

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains  Test Query Special

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Check Livequery Log Contains  Executing query name: Test Query Special and corresponding id: firstcommand

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Livequery Log Contains  Successfully executed query with name: Test Query Special and corresponding id: firstcommand

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Livequery Log Contains  Query result: {

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Cloud Server Log Contains  cloudServer: LiveQuery response (firstcommand) = {  1
    Check Cloud Server Log Contains  "type": "sophos.mgt.response.RunLiveQuery",  1

    ${fake_cloud_log}=  Get Fake Cloud Log File Path
    Check String Matching Regex In File  ${fake_cloud_log}  \"queryMetaData\": {\"durationMillis\":[0-9]+,\"errorCode\":0,\"errorMessage\":\"OK\",\"rows\":[0-9]+,\"sizeBytes\":[0-9]+},

    Check Cloud Server Log Contains  "columnMetaData": [{"name":"name","type":"TEXT"}],  1
    Check Cloud Server Log Contains  "columnData": [["systemd"],  1

EDR Does Not Trigger Query On Update Now Action
    [Tags]  EDR_PLUGIN  MANAGEMENT_AGENT
    Run Full Installer
    Install EDR Directly
    Wait Until EDR OSQuery Running

    ${edr_log} =  Get File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    ${edr_length_1} =  Get Length  ${edr_log}

    create file  ${SOPHOS_INSTALL}/tmp/ALC_action_timestamp.xml  content="content"
    move file  ${SOPHOS_INSTALL}/tmp/ALC_action_timestamp.xml  /opt/sophos-spl/base/mcs/action

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Management Agent Log Contains  Action ALC_action_timestamp.xml sent to 1 plugins

    ${edr_log} =  Get File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    ${edr_length_2} =  Get Length  ${edr_log}

    # Edr Should Not Have logged anything
    Should Be Equal  ${edr_length_1}  ${edr_length_2}

EDR plugin restarts mtr extension when killed
    [Tags]  EDR_PLUGIN
    Run Full Installer
    Override LogConf File as Global Level  DEBUG
    Install EDR Directly
    Wait Until EDR OSQuery Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log   edr_osquery_log   Created and monitoring extension child  1
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/SophosMTRExtension.log   mtr_ext_log  Finished registering tables  1

    ${result} =  Run Process  pgrep edr | xargs kill -9  shell=true

    Wait Until Keyword Succeeds
    ...  70 secs
    ...  10 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log   edr_osquery_log   Created and monitoring extension child  2
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/SophosMTRExtension.log   mtr_ext_log  Finished registering tables  2
    Run Live Query  ${GREP}   simple
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query  1

EDR Osquery restarts mtr extension when killed
    [Tags]  EDR_PLUGIN
    Run Full Installer
    Override LogConf File as Global Level  DEBUG
    Install EDR Directly
    Wait Until EDR OSQuery Running
    Run Live Query  ${GREP}   simple
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query  1
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/SophosMTRExtension.log   mtr_ext_log  Finished registering tables  1

    ${result} =  Run Process  pgrep MTR.ext | xargs kill -9  shell=true

    Wait Until Keyword Succeeds
    ...  70 secs
    ...  10 secs
    ...  Check EDR Log Contains  Increment telemetry: mtr-extension-restarts
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log   edr_osquery_log  Created and monitoring extension child  2
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/SophosMTRExtension.log   mtr_ext_log  Finished registering tables  2
    Run Live Query  ${GREP}   simple
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query  2
    ${result} =  Run Process  pgrep osquery | xargs kill -9  shell=true
    Wait Until Keyword Succeeds
    ...  70 secs
    ...  10 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log   edr_osquery_log  Created and monitoring extension child  3
        Wait Until Keyword Succeeds
        ...  5 secs
        ...  1 secs
        ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/SophosMTRExtension.log   mtr_ext_log  Finished registering tables  3
    Run Live Query  ${GREP}   simple
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query  3


