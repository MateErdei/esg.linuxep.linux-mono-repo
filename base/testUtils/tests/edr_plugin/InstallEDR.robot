*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       EDR Test Setup
Test Teardown    EDR Test Teardown

Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    EDRResources.robot

Default Tags   EDR_PLUGIN   OSTIA  FAKE_CLOUD   THIN_INSTALLER  INSTALLER


*** Variables ***
${BaseAndMtrReleasePolicy}          ${GeneratedWarehousePolicies}/base_and_mtr_VUT-1.xml
${BaseAndEdrVUTPolicy}              ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${BaseVUTPolicy}                    ${GeneratedWarehousePolicies}/base_only_VUT.xml
${EDR_STATUS_XML}                   ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
${EDR_PLUGIN_PATH}                  ${SOPHOS_INSTALL}/plugins/edr
${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
${CACHED_STATUS_XML} =              ${SOPHOS_INSTALL}/base/mcs/status/cache/LiveQuery.xml

*** Test Cases ***
Install EDR and handle Live Query
    Install EDR

    Run Shell Process   /opt/sophos-spl/bin/wdctl stop edr     OnError=Failed to stop edr
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start edr    OnError=Failed to start edr

    Send Query From Fake Cloud    Test Query Special   select name from processes   command_id=firstcommand
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains   /commands/applications/MCS;ALC;AGENT;LiveQuery;APPSPROXY

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Envelope Log Contains       Test Query Special

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Check EDR Log Contains    Executing query name: Test Query Special and corresponding id: firstcommand

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check EDR Log Contains    Successfully executed query with name: Test Query Special and corresponding id: firstcommand

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check EDR Log Contains    Query result: {

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Cloud Server Log Contains   cloudServer: LiveQuery response (firstcommand) = {  1
    Check Cloud Server Log Contains    "type": "sophos.mgt.response.RunLiveQuery",  1

    ${fake_cloud_log}=  Get Fake Cloud Log File Path
    Check String Matching Regex In File   ${fake_cloud_log}    \"queryMetaData\": {\"durationMillis\":[0-9]+,\"errorCode\":0,\"errorMessage\":\"OK\",\"rows\":[0-9]+,\"sizeBytes\":[0-9]+},

    Check Cloud Server Log Contains    "columnMetaData": [{"name":"name","type":"TEXT"}],  1
    Check Cloud Server Log Contains    "columnData": [["systemd"],  1

Install EDR And Get Historic Event Data
    Install EDR

    Run Shell Process   /opt/sophos-spl/bin/wdctl stop edr     OnError=Failed to stop edr
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start edr    OnError=Failed to start edr

    Wait Until OSQuery Running

    Wait Until Keyword Succeeds
    ...   7x
    ...   10 secs
    ...   Run Query Until It Gives Expected Results  select pid from process_events LIMIT 1  {"columnMetaData":[{"name":"pid","type":"BIGINT"}],"queryMetaData":{"errorCode":0,"errorMessage":"OK","rows":1}}

EDR Uninstaller Does Not Report That It Could Not Remove EDR If Watchdog Is Not Running
    [Teardown]  EDR Uninstall Teardown
    Install EDR
    ${systemctlResult} =  Run Process   systemctl stop sophos-spl   shell=yes
    Check Watchdog Not Running
    Should Be Equal As Strings  ${systemctlResult.rc}  0

    ${result} =  Uninstall EDR Plugin
    Should Not Contain  ${result.stderr}  Failed to remove edr: Watchdog is not running

EDR Removes Ipc And Status Files When Uninstalled
    Run Full Installer
    Install EDR Directly
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

EDR Uninstalled When Removed From ALC Policy
    Install EDR
    Wait Until OSQuery Running

    Send ALC Policy And Prepare For Upgrade  ${BaseVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
        ...   120 secs
        ...   20 secs
        ...   Check Suldownloader Log Contains In Order     Uninstalling plugin ServerProtectionLinux-Plugin-EDR

    Should Not Exist  ${EDR_DIR}

    # Similarly, "File Should Not Exist" will always pass on ${IPC_FILE}
    Should Not Exist        ${IPC_FILE}
    File Should Not Exist   ${EDR_STATUS_XML}
    File Should Not Exist   ${CACHED_STATUS_XML}

Install Base And MTR Then Migrate To EDR
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   EDR_PLUGIN
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrReleasePolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrReleasePolicy}  real=True
    Wait For Initial Update To Fail

    # Install MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrReleasePolicy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Should Exist  ${MTR_DIR}

    Check EDR Executable Not Running
    Should Not Exist  ${EDR_DIR}

    # Install EDR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  should exist  ${EDR_DIR}

    Wait Until EDR Running
    Wait Until OSQuery Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains    Uninstalling plugin ServerProtectionLinux-Plugin-MDR since it was removed from warehouse
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SophosMTR Executable Not Running
    # MTR is uninstalled after EDR is installed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Not Exist  ${MTR_DIR}

Install Base And EDR Then Migrate To BASE
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   EDR_PLUGIN
    Install EDR

    # Uninstall EDR
    Send ALC Policy And Prepare For Upgrade  ${BaseVUTPolicy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  should Not exist  ${EDR_DIR}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains    Uninstalling plugin ServerProtectionLinux-Plugin-EDR since it was removed from warehouse

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Is Not Running
