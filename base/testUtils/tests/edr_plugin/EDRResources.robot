*** Settings ***
Library    ${libs_directory}/FullInstallerUtils.py

*** Keywords ***
Wait For EDR to be Installed
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   10 secs
    ...   File Should exist    ${SOPHOS_INSTALL}/plugins/edr/bin/edr


    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   EDR Plugin Is Running

EDR Plugin Is Running
    ${result} =    Run Process  pgrep  edr
    Should Be Equal As Integers    ${result.rc}    0

Install EDR
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

EDR Suite Setup
    UpgradeResources.Suite Setup
    ${result} =  Run Process  curl -v https://ostia.eng.sophos/latest/sspl-warehouse/master   shell=True
    Should Be Equal As Integers  ${result.rc}  0  "Failed to Verify connection to Update Server. Please, check endpoint is configured. (Hint: tools/setup_sspl/setupEnvironment2.sh).\n StdOut: ${result.stdout}\n StdErr: ${result.stderr}"

EDR Suite Teardown
    UpgradeResources.Suite Teardown

EDR Test Setup
    UpgradeResources.Test Setup

Report Audit Link Ownership
    ${result} =  Run Process   auditctl -s   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}


EDR Test Teardown
    Run Keyword if Test Failed    Log File   ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.INFO
    Run Keyword if Test Failed    Report Audit Link Ownership
    Run Keyword if Test Failed    Report On MCS_CA
    Run Keyword if Test Failed    Log File  ${SOPHOS_INSTALL}/base/update/var/update_config.json

    UpgradeResources.Test Teardown   UninstallAudit=False

EDR Uninstall Teardown
    Require Watchdog Running
    Test Teardown

Uninstall EDR Plugin
    ${result} =  Run Process     ${EDR_PLUGIN_PATH}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

Wait For EDR Status
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml

Install EDR Directly
    ${EDR_SDDS_DIR} =  Get SSPL EDR Plugin SDDS
    ${result} =    Run Process  bash -x ${EDR_SDDS_DIR}/install.sh   shell=True
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}"
    Log  ${result.stdout}
    Log  ${result.stderr}
    Wait For EDR to be Installed