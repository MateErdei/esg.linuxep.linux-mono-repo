*** Settings ***
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Resource  ../upgrade_product/UpgradeResources.robot


*** Variables ***
# Telemetry variables
${COMPONENT_TEMP_DIR}  /tmp/runtimedetections_component
${RUNTIMEDETECTIONS_PLUGIN_PATH}    ${SOPHOS_INSTALL}/plugins/runtimedetections
${RUNTIMEDETECTIONS_EXECUTABLE}     ${RUNTIMEDETECTIONS_PLUGIN_PATH}/bin/runtimedetections


*** Keywords ***
Wait For RuntimeDetections to be Installed
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   10 secs
    ...   File Should exist    ${RUNTIMEDETECTIONS_EXECUTABLE}

    Wait Until Keyword Succeeds
    ...   40 secs
    ...   2 secs
    ...   RuntimeDetections Plugin Is Running


RuntimeDetections Plugin Is Running
    ${result} =    Run Process  pgrep  -f  ${RUNTIMEDETECTIONS_EXECUTABLE}
    Should Be Equal As Integers    ${result.rc}    0

RuntimeDetections Plugin Is Not Running
    ${result} =    Run Process  pgrep  -f  ${RUNTIMEDETECTIONS_EXECUTABLE}
    Should Not Be Equal As Integers    ${result.rc}    0   RuntimeDetections Plugin still running

Wait Keyword Succeed
    [Arguments]  ${keyword}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  ${keyword}

Install RuntimeDetections Directly
    ${RUNTIMEDETECTIONS_SDDS_DIR} =  Get SSPL RuntimeDetections Plugin SDDS
    ${result} =    Run Process  bash -x ${RUNTIMEDETECTIONS_SDDS_DIR}/install.sh 2> /tmp/rtd_install.log   shell=True
    ${stderr} =   Get File  /tmp/rtd_install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr} ${stderr}"
    Log  ${result.stdout}
    Log  ${stderr}
    Log  ${result.stderr}
    Wait For RuntimeDetections to be Installed