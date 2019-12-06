*** Settings ***
Library         Process

*** Variables ***
${EDR_PLUGIN_PATH}                  ${SOPHOS_INSTALL}/plugins/edr
${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc

*** Keywords ***
#Install EDR Directly
#    ${MDR_SDDS_DIR} =  Get SSPL MDR Plugin SDDS
#    ${result} =    Run Process  ${MDR_SDDS_DIR}/install.sh
#    Should Be Equal As Integers    ${result.rc}    0
#    Log  ${result.stdout}
#    Log  ${result.stderr}
#    Check MDR Plugin Installed

Check MDR Plugin Installed
    File Should Exist   ${EDR_PLUGIN_PATH}/bin/edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  edr <> Entering the main loop