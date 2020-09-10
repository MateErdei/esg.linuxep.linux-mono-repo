*** Settings ***
Documentation   Product tests for sophos_threat_detector
Default Tags    PRODUCT

Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/FakeManagement.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Test Teardown  Threat Detector Test Teardown

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log

*** Keywords ***
Threat Detector Test Teardown
    ${result} =  Run Process  ls  -lR  ${AV_PLUGIN_PATH}
    Log Many  stdout: ${result.stdout}  stderr: ${result.stderr}
    Component Test TearDown

Start AV
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Check AV Plugin Installed
    # wait for AV Plugin to initialize
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  2 secs
        ...  Threat Detector Log Contains  UnixSocket <> Starting listening on socket

Stop AV
     ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}

Verify threat detector log rotated
    List Directory  ${AV_PLUGIN_PATH}/log/sophos_threat_detector
    Should Exist  ${AV_PLUGIN_PATH}/log/sophos_threat_detector/sophos_threat_detector.log.1

*** Test Cases ***

Threat Detector Log Rotates
    Start AV
    Stop AV
    Increase Threat Detector Log To Max Size
    Start AV
    Stop AV
    Verify threat detector log rotated
