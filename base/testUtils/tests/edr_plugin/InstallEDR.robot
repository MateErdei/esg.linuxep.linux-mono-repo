*** Settings ***
Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

Test Setup       Test Setup
Test Teardown    Test Teardown

Library     ${libs_directory}/WarehouseUtils.py
Library     ${libs_directory}/ThinInstallerUtils.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/MCSRouter.py

Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    EDRResources.robot


*** Variables ***
${BaseAndEdrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml

*** Test Cases ***
Install EDR and handle Live Query
    [Tags]  EDR_PLUGIN   OSTIA  FAKE_CLOUD   THIN_INSTALLER  INSTALLER
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}
    Wait For Initial Update To Fail

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   20 secs
    ...   Check Suldownloader Log Contains In Order     Installing product: ServerProtectionLinux-Plugin-EDR   Generating the report file

    Wait For EDR to be Installed

    Should Exist  ${EDR_DIR}
    Wait Until OSQuery Running

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
    ...  10 secs
    ...  2 secs
    ...  Check Cloud Server Log Contains   cloudServer: LiveQuery response (firstcommand) = {  1
    Check Cloud Server Log Contains    "type": "sophos.mgt.response.RunLiveQuery",  1
    Check Cloud Server Log Contains      "queryMetaData": {"errorCode":0,"errorMessage":"OK","rows":   1
    Check Cloud Server Log Contains    "columnMetaData": [{"name":"name","type":"TEXT"}],  1
    Check Cloud Server Log Contains    "columnData": [["systemd"],  1


Install EDR And Get Historic Event Data
    [Tags]  EDR_PLUGIN   OSTIA  FAKE_CLOUD   THIN_INSTALLER  INSTALLER
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}
    Wait For Initial Update To Fail

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   20 secs
    ...   Check Suldownloader Log Contains In Order     Installing product: ServerProtectionLinux-Plugin-EDR   Generating the report file

    Wait For EDR to be Installed
    Should Exist  ${EDR_DIR}

    Run Shell Process   /opt/sophos-spl/bin/wdctl stop edr     OnError=Failed to stop edr
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start edr    OnError=Failed to start edr

    Wait Until OSQuery Running

    Wait Until Keyword Succeeds
    ...   7x
    ...   10 secs
    ...   Run Query Until It Gives Expected Results  select pid from process_events LIMIT 1  {"columnMetaData":[{"name":"pid","type":"BIGINT"}],"queryMetaData":{"errorCode":0,"errorMessage":"OK","rows":1}}


*** Keywords ***

Run Query Until It Gives Expected Results
    [Arguments]  ${query}  ${expectedResponseJson}
    Send Query From Fake Cloud    ProcessEvents  ${query}  command_id=queryname
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Last Live Query Response   ${expectedResponseJson}

Report On MCS_CA
    ${mcs_ca} =  Get Environment Variable  MCS_CA
    ${result}=  Run Process    ls -l ${mcs_ca}     shell=True
    Log  ${result.stdout}
    Log File  ${mcs_ca}

Suite Setup
    UpgradeResources.Suite Setup
    ${result} =  Run Process  curl -v https://ostia.eng.sophos/latest/sspl-warehouse/master   shell=True
    Should Be Equal As Integers  ${result.rc}  0  "Failed to Verify connection to Update Server. Please, check endpoint is configured. (Hint: tools/setup_sspl/setupEnvironment2.sh).\n StdOut: ${result.stdout}\n StdErr: ${result.stderr}"

Suite Teardown
    UpgradeResources.Suite Teardown

Test Setup
    UpgradeResources.Test Setup

Report Audit Link Ownership
    ${result} =  Run Process   auditctl -s   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}


Test Teardown
    Run Keyword if Test Failed    Log File   ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.INFO
    Run Keyword if Test Failed    Report Audit Link Ownership
    Run Keyword if Test Failed    Report On MCS_CA
    Run Keyword if Test Failed    Log File  ${SOPHOS_INSTALL}/base/update/var/update_config.json

    UpgradeResources.Test Teardown   UninstallAudit=False
