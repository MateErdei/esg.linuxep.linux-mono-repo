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
Resource    ../liveresponse_plugin/LiveResponseResources.robot

Default Tags   EDR_PLUGIN   FAKE_CLOUD   THIN_INSTALLER  INSTALLER


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
    Wait Until OSQuery Running
    ${systemctlResult} =  Run Process   systemctl stop sophos-spl   shell=yes
    Check Watchdog Not Running
    Should Be Equal As Strings  ${systemctlResult.rc}  0

    ${result} =  Uninstall EDR Plugin
    Should Not Contain  ${result.stderr}  Failed to remove edr: Watchdog is not running

EDR Removes Ipc And Status Files When Uninstalled
    Run Full Installer
    Install EDR Directly
    Wait Until OSQuery Running
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

EDR Does Not Trigger Query On Update Now Action
    [Tags]  EDR_PLUGIN  MANAGEMENT_AGENT
    Run Full Installer
    Install EDR Directly
    Wait Until OSQuery Running

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

EDR runs sophos extension query
    Run Full Installer
    Override LogConf File as Global Level  DEBUG
    Install EDR Directly
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config    MCSID=id-here
    Wait Until OSQuery Running
    sleep  60
#    Wait Until Keyword Succeeds
#    ...  100 secs
#    ...  2 secs
#    ...  Check EDR Log Contains   "log message that the extension has been started"
    Run Live Query  ${SOPHOS_INFO_QUERY}  sophos_info
    Wait Until Keyword Succeeds
    ...  50 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query with name: sophos_info  1
    Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log   "columnData": [["id-here"]]  1

*** Keywords ***
EDR Tests Teardown With Installed File Replacement
    Run Keyword If Test Failed  Save Current EDR InstalledFiles To Local Path
    EDR Test Teardown

Save Current EDR InstalledFiles To Local Path
    Create File  ${ROBOT_TESTS_DIR}/edr_plugin/InstallSet/FileInfo  ${FileInfo}
    Create File  ${ROBOT_TESTS_DIR}/edr_plugin/InstallSet/DirectoryInfo  ${DirectoryInfo}
    Create File  ${ROBOT_TESTS_DIR}/edr_plugin/InstallSet/SymbolicLinkInfo  ${SymbolicLinkInfo}

