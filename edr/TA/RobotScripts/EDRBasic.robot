*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py

Resource    ComponentSetup.robot

*** Variables ***
${EDR_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${EDR_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${EDR_LOG_PATH}    ${EDR_PLUGIN_PATH}/log/edr.log

*** Test Cases ***
EDR Plugin Can Register With Management Agent And Recieve A Policy
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}

    Check EDR Plugin Installed

    ${policyContent} =  Set Variable   This is a policy test only
    Send Plugin Policy  edr  LiveQuery  ${policyContent}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  ${policyContent}

EDR Plugin Can Recieve Actions
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}

    Check EDR Plugin Installed

    ${actionContent} =  Set Variable  This is an action test
    Send Plugin Action  edr  LiveQuery  ${actionContent}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Received new Action

EDR plugin Can Send Status
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}
    Check EDR Plugin Installed

    ${edrStatus}=  Get Plugin Status  edr  LiveQuery
    Should Contain  ${edrStatus}   RevID

    ${edrTelemetry}=  Get Plugin Telemetry  edr
    Should Contain  ${edrTelemetry}   Number of Scans

    ${result} =   Terminate Process  ${handle}


*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}
    ${result} =   Run Process  ${Command}   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\n stdout: \n ${result.stdout} \n. stderr: \n {result.stderr}"

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

