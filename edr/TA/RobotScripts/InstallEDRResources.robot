*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py


Test Teardown   Remove All
*** Variables ***
${TEST_INPUT_PATH}  /opt/test/inputs/edr
${BASE_SDDS}    ${TEST_INPUT_PATH}/base-sdds
${EDR_SDDS}     ${TEST_INPUT_PATH}/SDDS-COMPONENT
${SOPHOS_INSTALL}   /opt/sophos-spl
${EDR_PLUGIN_PATH}  ${SOPHOS_INSTALL}/plugins/edr
${FAKEMANAGEMENT_LOG_PATH}  ${SOPHOS_INSTALL}/tmp/fake_management_agent.log
${EDR_PLUGIN_BIN}   ${EDR_PLUGIN_PATH}/bin/edr
${EDR_LOG_PATH}    ${EDR_PLUGIN_PATH}/log/edr.log

${EDR_IPC_FILE}       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc

*** Test Cases ***
EDR Can Be Installed and Executed By Watchdog
    Mock Base For Component Installed
    Copy EDR Components
    Start Fake Management
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}

    Check EDR Plugin Installed

    ${policyContent} =  Set Variable   This is a policy test only
    send plugin policy  edr  LiveQuery  ${policyContent}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  ${policyContent}
    ${actionContent} =  Set Variable  This is an action test
    send plugin action  edr  LiveQuery  ${actionContent}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Received new Action

    ${edrStatus}=  get plugin status  edr  LiveQuery
    Should Contain  ${edrStatus}   RevID

    ${edrTelemetry}=  get plugin telemetry  edr
    Should Contain  ${edrTelemetry}   Number of Scans

    ${result} =   Terminate Process  ${handle}


*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}
    ${result} =   Run Process  ${Command}   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\n stdout: \n ${result.stdout} \n. stderr: \n {result.stderr}"


Mock Base For Component Installed
    Create Directory   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/tmp
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/plugins/
    Create File        ${SOPHOS_INSTALL}/base/etc/logger.conf   VERBOSITY=DEBUG
    Run Process   groupadd  sophos-spl-group


Copy EDR Components
    Copy Directory   ${EDR_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    Copy Directory   ${EDR_SDDS}/files/base/pluginRegistry   ${SOPHOS_INSTALL}
    Run Process   ldconfig   -lN   *.so.*   cwd=${EDR_PLUGIN_PATH}/lib64/   shell=True
    Run Process   chmod +x ${EDR_PLUGIN_BIN}  shell=True


Remove All
    Stop Fake Management
    Terminate All Processes  kill=True
    Log File   ${EDR_LOG_PATH}
    Log File   ${SOPHOS_INSTALL}/tmp/fake_management_agent.log
    Run Process   rm   -rf   ${SOPHOS_INSTALL}


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
    File Log Contains  ${FAKEMANAGEMENT_LOG_PATH}   ${input}

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
