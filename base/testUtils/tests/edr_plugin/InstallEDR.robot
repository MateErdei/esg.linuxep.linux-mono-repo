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
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    ../watchdog/LogControlResources.robot
Resource    EDRResources.robot

Default Tags   EDR_PLUGIN   OSTIA  FAKE_CLOUD   THIN_INSTALLER  INSTALLER


*** Variables ***
${BaseAndMtrReleasePolicy}          ${GeneratedWarehousePolicies}/base_and_mtr_VUT-1.xml
${BaseAndMtrVUTPolicy}              ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseAndEdrVUTPolicy}              ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${BaseAndEdrAndMtrVUTPolicy}        ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml
${BaseAndEdr999Policy}              ${GeneratedWarehousePolicies}/base_and_edr_999.xml
${BaseEdrAndMtr999Policy}              ${GeneratedWarehousePolicies}/base_edr_vut_and_mtr_999.xml
${BaseMtrAndEdr999Policy}              ${GeneratedWarehousePolicies}/base_mtr_vut_and_edr_999.xml
${BaseAndMTREdr999Policy}              ${GeneratedWarehousePolicies}/base_vut_and_mtr_edr_999.xml
${BaseVUTPolicy}                    ${GeneratedWarehousePolicies}/base_only_VUT.xml
${EDR_STATUS_XML}                   ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
${EDR_PLUGIN_PATH}                  ${SOPHOS_INSTALL}/plugins/edr
${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc
${CACHED_STATUS_XML} =              ${SOPHOS_INSTALL}/base/mcs/status/cache/LiveQuery.xml
${SULDOWNLOADER_LOG_PATH}           ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${WDCTL_LOG_PATH}                   ${SOPHOS_INSTALL}/logs/base/wdctl.log

*** Test Cases ***
Install EDR and handle Live Query
    Install EDR  ${BaseAndEdrVUTPolicy}
    Wait Until OSQuery Running

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
    Install EDR  ${BaseAndEdrVUTPolicy}
    Wait Until OSQuery Running
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
    Install EDR  ${BaseAndEdrVUTPolicy}
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

EDR Uninstalled When Removed From ALC Policy
    Install EDR  ${BaseAndEdrVUTPolicy}
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
    Install EDR  ${BaseAndEdrVUTPolicy}
    Wait Until OSQuery Running
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

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

Install base and edr 999 then downgrade to current master
    Install EDR  ${BaseAndEdr999Policy}
    Wait Until OSQuery Running

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog

    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.0.0
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Is Running

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should not contain   ${contents}   PRODUCT_VERSION = 9.99.9

    # Ensure EDR was restarted during upgrade.
    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop edr
    ...  wdctl <> start edr

Install base and edr and mtr then downgrade to current release base and mtr
    [Tags]  TESTFAILURE
    #TODO LINUXDAR-1196 remove testfailure tag when we next release
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}
    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrReleasePolicy}
    #truncate log so that check mdr plugin installed works correctly later in the test
    ${result} =  Run Process   truncate   -s   0   ${MTR_DIR}/log/mtr.log
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Uninstalling plugin ServerProtectionLinux-Plugin-EDR since it was removed from warehouse

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  EDR Plugin Is Not Running

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Should Not Exist  ${EDR_DIR}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Exist  ${MTR_DIR}

    Check MDR Plugin Installed

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3


Install master of base and edr and mtr and upgrade to mtr 999
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 1.0.0
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader

    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtr999Policy}
    #truncate log so that check mdr plugin installed works correctly later in the test
    ${result} =  Run Process   truncate   -s   0   ${MTR_DIR}/log/mtr.log
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Is Running

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3
    Check MDR Plugin Installed

    ${mtr_version_contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should contain   ${mtr_version_contents}   PRODUCT_VERSION = 9.99.9

    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should not contain   ${edr_version_contents}   PRODUCT_VERSION = 9.99.9


Update Run that Does Not Change The Product Does not ReInstall The Product
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 1.0.0

    Prepare Installation For Upgrade Using Policy   ${BaseAndEdrAndMtrVUTPolicy}

    Override LogConf File as Global Level  DEBUG

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Generating the report file in   2

    Run Keyword And Expect Error   *1 times not the requested 2 times*   Upgrade Installs EDR Twice

    Check MDR Plugin Installed

    Fail


Install master of base and edr and mtr and upgrade to edr 999
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.0.0
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog

    Send ALC Policy And Prepare For Upgrade  ${BaseMtrAndEdr999Policy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Is Running

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${edr_version_contents}   PRODUCT_VERSION = 9.99.9

    ${mtr_version_contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should not contain   ${mtr_version_contents}   PRODUCT_VERSION = 9.99.9

    # Ensure EDR was restarted during upgrade.
    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop edr
    ...  wdctl <> start edr



Install master of base and edr and mtr and upgrade to edr 999 and mtr 999
    Install EDR  ${BaseAndEdrAndMtrVUTPolicy}

    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 1.0.0
    Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 1.0.0
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader
    Check Log Does Not Contain    Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9     ${SULDOWNLOADER_LOG_PATH}  Sul-Downloader

    Check Log Does Not Contain    wdctl <> stop edr     ${WDCTL_LOG_PATH}  WatchDog

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMTREdr999Policy}
    #truncate log so that check mdr plugin installed works correctly later in the test
    ${result} =  Run Process   truncate   -s   0   ${MTR_DIR}/log/mtr.log
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR version: 9.99.9
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR version: 9.99.9
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Is Running

    Check MDR Plugin Installed
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

    ${edr_version_contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${edr_version_contents}   PRODUCT_VERSION = 9.99.9
    ${mtr_version_contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should contain   ${mtr_version_contents}   PRODUCT_VERSION = 9.99.9

    Check Log Contains In Order
    ...  ${WDCTL_LOG_PATH}
    ...  wdctl <> stop edr
    ...  wdctl <> start edr


Install Base And Mtr Vut Then Transition To Base Edr And Mtr Vut
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrVUTPolicy}
    Wait For Initial Update To Fail

    # Install MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   1

    # ensure MTR plugin is installed and running
    Wait For MDR To Be Installed

    # ensure EDR is not installed.
    Wait Until EDR Uninstalled

    # Install EDR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-EDR

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   2

    Wait For EDR to be Installed

    # ensure Plugins are running after update
    Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MTR Osquery Executable Running
    EDR Plugin Is Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Osquery Executable Running

Install Base And Edr Vut Then Transition To Base Edr And Mtr Vut
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}
    Wait For Initial Update To Fail

    # Install EDR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   1

    # ensure EDR plugin is installed and running
    Wait For EDR to be Installed

    # ensure MTR is not installed.
    Wait Until MDR Uninstalled

    # Install MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Installing product: ServerProtectionLinux-Plugin-MDR

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   2

    Wait Until MDR Installed

    # ensure Plugins are running after update
    Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MTR Osquery Executable Running
    EDR Plugin Is Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Osquery Executable Running


Install Base Edr And Mtr Vut Then Transition To Base Mtr Vut
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrAndMtrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrAndMtrVUTPolicy}
    Wait For Initial Update To Fail

    # Install EDR And MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   1

    # ensure initial plugins are installed and running
    Wait Until MDR Installed
    Wait For EDR to be Installed

    # Transition to MTR Only
    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Uninstalling plugin ServerProtectionLinux-Plugin-EDR since it was removed from warehouse

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   2

    Wait Until EDR Uninstalled

    # ensure MTR still running after update
    Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MTR Osquery Executable Running


Install Base Edr And Mtr Vut Then Transition To Base Edr Vut
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrAndMtrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrAndMtrVUTPolicy}
    Wait For Initial Update To Fail

    # Install EDR And MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrAndMtrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   1

    # ensure initial plugins are installed and running
    Wait Until MDR Installed
    Wait For EDR to be Installed

    # Transition to EDR Only
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains     Uninstalling plugin ServerProtectionLinux-Plugin-MDR since it was removed from warehouse

    Wait Until MDR Uninstalled

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Update success   2

    # ensure EDR still running after update
    EDR Plugin Is Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Osquery Executable Running


*** Keywords ***
Upgrade Installs EDR Twice   
    Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log   Installing product: ServerProtectionLinux-Plugin-EDR   2