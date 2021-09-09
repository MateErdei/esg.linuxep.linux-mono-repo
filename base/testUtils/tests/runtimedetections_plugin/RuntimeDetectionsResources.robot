*** Settings ***
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Resource  ../upgrade_product/UpgradeResources.robot


*** Variables ***
# Telemetry variables
${COMPONENT_TEMP_DIR}  /tmp/runtimedetections_component
${RUNTIMEDETECTIONS_PLUGIN_PATH}    ${SOPHOS_INSTALL}/plugins/runtimedetections



*** Keywords ***
Wait For RuntimeDetections to be Installed
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   10 secs
    ...   File Should exist    ${SOPHOS_INSTALL}/plugins/runtimedetections/bin/capsule8-sensor

    Wait Until Keyword Succeeds
    ...   40 secs
    ...   2 secs
    ...   RuntimeDetections Plugin Is Running


RuntimeDetections Plugin Is Running
    ${result} =    Run Process  pgrep  capsule8-sensor
    Should Be Equal As Integers    ${result.rc}    0

RuntimeDetections Plugin Is Not Running
    ${result} =    Run Process  pgrep  capsule8-sensor
    Should Not Be Equal As Integers    ${result.rc}    0   RuntimeDetections PLugin still running

Restart RuntimeDetections Plugin
    [Arguments]  ${clearLog}=False
    Wdctl Stop Plugin  capsule8-sensor
    Run Keyword If   ${clearLog}   Remove File  ${RUNTIMEDETECTIONS_DIR}/log/runtimedetections.log
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   RuntimeDetections Plugin Is Not Running
    Wdctl Start Plugin  capsule8-sensor
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   RuntimeDetections Plugin Is Running

Install RuntimeDetections
    [Arguments]  ${policy}  ${args}=${None}
    Start Local Cloud Server  --initial-alc-policy  ${policy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${policy}  args=${args}

    Send ALC Policy And Prepare For Upgrade  ${policy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check Suldownloader Log Contains In Order     Installing product: ServerProtectionLinux-Plugin-RuntimeDetections   Generating the report file

    Wait For RuntimeDetections to be Installed

    Should Exist  ${RUNTIMEDETECTIONS_DIR}

Wait Keyword Succeed
    [Arguments]  ${keyword}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  ${keyword}

RuntimeDetections Suite Setup
    UpgradeResources.Suite Setup
    ${result} =  Run Process  curl -v https://ostia.eng.sophos/latest/sspl-warehouse/master   shell=True
    Should Be Equal As Integers  ${result.rc}  0  "Failed to Verify connection to Update Server. Please, check endpoint is configured. (Hint: tools/setup_sspl/setupEnvironment2.sh).\nStdOut: ${result.stdout}\nStdErr: ${result.stderr}"

RuntimeDetections Suite Teardown
    UpgradeResources.Suite Teardown

RuntimeDetections Test Setup
    UpgradeResources.Test Setup

RuntimeDetections Uninstall Teardown
    Require Watchdog Running
    Test Teardown

Uninstall RuntimeDetections Plugin
    ${result} =  Run Process     ${RUNTIMEDETECTIONS_PLUGIN_PATH}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

#Wait For RuntimeDetections Status
#    Wait Until Keyword Succeeds
#    ...  30
#    ...  1
#    ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml

Install RuntimeDetections Directly
    ${RUNTIMEDETECTIONS_SDDS_DIR} =  Get SSPL RuntimeDetections Plugin SDDS
    ${result} =    Run Process  bash -x ${RUNTIMEDETECTIONS_SDDS_DIR}/install.sh 2> /tmp/c8_install.log   shell=True
    ${stderr} =   Get File  /tmp/c8_install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr} ${stderr}"
    Log  ${result.stdout}
    Log  ${stderr}
    Log  ${result.stderr}
    Wait For RuntimeDetections to be Installed

Check RuntimeDetections Plugin Uninstalled
    RuntimeDetections Plugin Is Not Running
    Should Not Exist  ${RUNTIMEDETECTIONS_DIR}

Wait Until RuntimeDetections Uninstalled
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  Check RuntimeDetections Plugin Uninstalled