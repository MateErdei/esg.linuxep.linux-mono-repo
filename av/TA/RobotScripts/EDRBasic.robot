*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py

Resource    ComponentSetup.robot
Resource    EDRResources.robot

*** Variables ***
${EDR_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${EDR_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${EDR_LOG_PATH}    ${EDR_PLUGIN_PATH}/log/edr.log

*** Test Cases ***
EDR Plugin Can Recieve Actions
    ${handle} =  Start Process  ${EDR_PLUGIN_BIN}

    Check EDR Plugin Installed

    ${actionContent} =  Set Variable  This is an action test
    Send Plugin Action  edr  LiveQuery  corr123  ${actionContent}
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

