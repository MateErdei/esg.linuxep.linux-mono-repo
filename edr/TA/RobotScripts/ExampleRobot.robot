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
    Log  Passing.
    ${result} =  Run Process  ls /opt/test/inputs/edr    shell=True
    Log to console  "what is inside /opt/test/inputs/edr"
    Log to console  ${result.stdout}
    Log to console  ${result.stderr}

    Install Base For Component Tests
    Install EDR Directly from SDDS
    Check EDR Plugin Installed


*** Keywords ***
Install Base For Component Tests
    Log to console  "run base installer"
    ${result} =   Run Process  bash ${BASE_SDDS}/install.sh   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install base.\n stdout: \n ${result.stdout} \n. stderr: \n {result.stderr}"
    Log to console  "stop mcsr router"
    ${result} =   Run Process  /opt/sophos-spl/bin/wdctl stop mcsrouter  shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to stop mcsrouter.\n stdout: \n ${result.stdout} \n. stderr: \n {result.stderr}"

Install EDR Directly from SDDS
    Log to console  "install edr"
    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install edr.\n stdout: \n${result.stdout}\n. stderr: \n {result.stderr}"

Check EDR Plugin Running
    ${result} =   Run Process   pidof ${SOPHOS_INSTALL}/plugins/edr/bin/edr   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "EDR not running.\n stdout: \n${result.stdout}\n. stderr: \n {result.stderr}"

EDR Plugin Log Contains
    [Arguments]  ${input}
    ${content} =  Get File   ${EDR_LOG_PATH}
    Should Contain  ${content}  ${input}

Check EDR Plugin Installed
    log to console  "check edr plugin installed"
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
    log to console  "uninstall all"
    Log File   ${EDR_LOG_PATH}
    Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall base.\n stdout: \n${result.stdout}\n. stderr: \n {result.stderr}"
