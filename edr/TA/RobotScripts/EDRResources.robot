*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py

Resource    ComponentSetup.robot

*** Variables ***
${EDR_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${EDR_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${EDR_LOG_PATH}     ${EDR_PLUGIN_PATH}/log/edr.log
${BASE_SDDS}        ${TEST_INPUT_PATH}/edr/base-sdds/
${EDR_SDDS}         ${COMPONENT_SDDS}

*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\n stdout: \n ${result.stdout} \n. stderr: \n ${result.stderr}"

Check EDR Plugin Running
    Run Shell Process  pidof ${SOPHOS_INSTALL}/plugins/edr/bin/edr   OnError=EDR not running

File Log Contains
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}
    Should Contain  ${content}  ${input}

EDR Plugin Log Contains
    [Arguments]  ${input}
    File Log Contains  ${EDR_LOG_PATH}   ${input}

FakeManagement Log Contains
    [Arguments]  ${input}
    File Log Contains  ${FAKEMANAGEMENT_AGENT_LOG_PATH}   ${input}

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
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  FakeManagement Log Contains   Registered plugin: edr

Simulate Live Query
    [Arguments]  ${name}  ${correlation}=123-4
    ${requestFile} =  Set Variable  ${ROBOT_SCRIPTS_PATH}/data/${name}
    File Should Exist  ${requestFile}
    Copy File   ${requestFile}  ${SOPHOS_INSTALL}/tmp
    Run Shell Process  chown sophos-spl-user:sophos-spl-group ${SOPHOS_INSTALL}/tmp/${name}  OnError=Failed to change ownership
    Move File    ${SOPHOS_INSTALL}/tmp/${name}  ${SOPHOS_INSTALL}/base/mcs/action/LiveQuery_${correlation}_FakeTime_request.json

Install With Base SDDS
    Remove Directory   ${SOPHOS_INSTALL}   recursive=True
    Directory Should Not Exist  ${SOPHOS_INSTALL}
    Install Base For Component Tests
    Install EDR Directly from SDDS

Uninstall All
    Log File    /tmp/installer.log
    Log File   ${EDR_LOG_PATH}
    Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to uninstall base.\n stdout: \n${result.stdout}\n. stderr: \n {result.stderr}"

Uninstall And Revert Setup
    Uninstall All
    Setup Base And Component

Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Shell Process   bash -x ${BASE_SDDS}/install.sh 2> /tmp/installer.log   OnError=Failed to Install Base   timeout=60s
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install EDR Directly from SDDS
    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install edr.\n stdout: \n${result.stdout}\n. stderr: \n {result.stderr}"

Check EDR Plugin Installed With Base
    File Should Exist   ${EDR_PLUGIN_PATH}/bin/edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check EDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  edr <> Entering the main loop

Check Osquery Running
    Run Shell Process  pidof ${SOPHOS_INSTALL}/plugins/edr/bin/osqueryd   OnError=osquery not running

Display All SSPL Files Installed
    ${handle}=  Start Process  find ${SOPHOS_INSTALL} | grep -v python | grep -v primarywarehouse | grep -v temp_warehouse | grep -v TestInstallFiles | grep -v lenses   shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

EDR And Base Teardown
    Run Keyword If Test Failed   Log File   ${EDR_LOG_PATH}
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword If Test Failed   Display All SSPL Files Installed
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains      edr <> Plugin Finished
    Run Keyword If Test Failed   Log File   ${EDR_LOG_PATH}
    Remove File    ${EDR_LOG_PATH}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr

Create Install Options File With Content
    [Arguments]  ${installFlags}
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  ${installFlags}
    #TODO set permissions