*** Settings ***

Test Setup       Require Uninstalled
Test Teardown    EDR Test Teardown

Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py

Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsPushClientResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot

Default Tags   EDR_PLUGIN   FAKE_CLOUD  INSTALLER
Force Tags  LOAD1


*** Variables ***
${EDR_STATUS_XML}                   ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
${CACHED_STATUS_XML} =              ${SOPHOS_INSTALL}/base/mcs/status/cache/LiveQuery.xml
${WDCTL_LOG_PATH}                   ${SOPHOS_INSTALL}/logs/base/wdctl.log

*** Test Cases ***
EDR Plugin Installs With Version Ini File
    Run Full Installer
    Install EDR Directly
    File Should Exist   ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini   SPL-Endpoint-Detection-and-Response-Plugin

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
    # Exclude on SLES until LINUXDAR-7033 is fixed
    [Tags]  EXCLUDE_UBUNTU18  EXCLUDE_SLES12  EXCLUDE_SLES15  EDR_PLUGIN   FAKE_CLOUD  INSTALLER
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
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  3 secs
    ...  Check Marked EDR Log Contains  SophosExtension running

    Send Query From Fake Cloud  Test Query Special  select name from processes  command_id=firstcommand
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains  /commands/applications/MCS;ALC;CORE;LiveQuery

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

    create file  ${MCS_TMP_DIR}/ALC_action_timestamp.xml  content="content"
    move file  ${MCS_TMP_DIR}/ALC_action_timestamp.xml  ${MCS_DIR}/action

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Management Agent Log Contains  Action ALC_action_timestamp.xml sent to 1 plugins

    ${edr_log} =  Get File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    ${edr_length_2} =  Get Length  ${edr_log}

    # Edr Should Not Have logged anything
    Should Be Equal  ${edr_length_1}  ${edr_length_2}



