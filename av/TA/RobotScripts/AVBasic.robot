*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py

Resource    ComponentSetup.robot
Resource    AVResources.robot

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log

*** Test Cases ***
AV Plugin Can Recieve Actions
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}

    Check AV Plugin Installed

    ${actionContent} =  Set Variable  This is an action test
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  AV Plugin Log Contains  Received new Action

    ${result} =   Terminate Process  ${handle}

AV plugin Can Send Status
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    ${status}=  Get Plugin Status  av  sav
    Should Contain  ${status}   RevID

    ${telemetry}=  Get Plugin Telemetry  av
    Should Contain  ${telemetry}   Number of Scans

    ${result} =   Terminate Process  ${handle}

