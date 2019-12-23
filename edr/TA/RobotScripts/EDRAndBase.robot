*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      No Operation
Test Teardown   EDR And Base Teardown

*** Variables ***
${BASE_SDDS}            ${TEST_INPUT_PATH}/edr/base-sdds/
${EDR_SDDS}             ${COMPONENT_SDDS}

*** Test Cases ***
LiveQuery is Distributed to EDR Plugin and Its Answer is available to MCSRouter
    Check EDR Plugin Installed With Base
    Simulate Live Query  RequestProcesses.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_123-4_response.json

*** Keywords ***
Simulate Live Query
    [Arguments]  ${name}  ${correlation}=123-4
    ${requestFile} =  Set Variable  RobotScripts/data/${name}
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


EDR And Base Teardown
    Run Keyword If Test Failed   Log File   ${EDR_LOG_PATH}
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Run Keyword If Test Failed   Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log
