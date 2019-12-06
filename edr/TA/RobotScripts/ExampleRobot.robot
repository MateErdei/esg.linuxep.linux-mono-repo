*** Settings ***
Library         Process
Library         OperatingSystem

Test Teardown   Uninstall All
*** Variables ***
${TEST_INPUT_PATH}  /opt/test/inputs/edr
${BASE_SDDS}    ${TEST_INPUT_PATH}/base-sdds
${EDR_SDDS}     ${TEST_INPUT_PATH}/SDDS-COMPONENT
${SOPHOS_INSTALL}   /opt/sophos-spl
${EDR_PLUGIN_PATH}  ${SOPHOS_INSTALL}/plugins/edr
${EDR_LOG_PATH}    ${EDR_PLUGIN_PATH}/log/edr.log

*** Test Cases ***
EDR Can Be Installed and Executed By Watchdog
    Install Base For Component Tests
    Install EDR Directly from SDDS
    Check EDR Plugin Installed


*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}
    ${result} =   Run Process  ${Command}   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\n stdout: \n ${result.stdout} \n. stderr: \n {result.stderr}"



Install Base For Component Tests
    Run Shell Process  bash ${BASE_SDDS}/install.sh    OnError=Base Installation Failed
    Run Shell Process  /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to Stop Mcsrouter

Install EDR Directly from SDDS
    Run Shell Process  bash ${EDR_SDDS}/install.sh   OnError=Failed to install EDR

Check EDR Plugin Running
    Run Shell Process  pidof ${SOPHOS_INSTALL}/plugins/edr/bin/edr   OnError=EDR not running

EDR Plugin Log Contains
    [Arguments]  ${input}
    ${content} =  Get File   ${EDR_LOG_PATH}
    Should Contain  ${content}  ${input}

Check EDR Plugin Installed
    File Should Exist   ${EDR_PLUGIN_PATH}/bin/edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check EDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  edr <> Entering the main loop

Uninstall All
    Log File   ${EDR_LOG_PATH}
    Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Shell Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   OnError=Failed to Uninstall base
