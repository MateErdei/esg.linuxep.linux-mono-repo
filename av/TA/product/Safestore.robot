*** Settings ***
Documentation   Product tests for SafeStore
Force Tags      PRODUCT  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Test Setup     Safestore Test Setup
Test Teardown  Safestore Test Teardown

*** Keywords ***

Safestore Test Setup
    Component Test Setup

Safestore Test Teardown
    Stop Safestore
    List AV Plugin Path
    run teardown functions
    Check All Product Logs Do Not Contain Error
    Dump and Reset Logs
    Component Test TearDown

Start Safestore
    ${handle} =  Start Process  ${SAFESTORE_BIN}  stdout=DEVNULL  stderr=DEVNULL
    Set Test Variable  ${SAFESTORE_HANDLE}  ${handle}
    Wait Until Safestore running

Stop Safestore
    ${result} =  Terminate Process  ${SAFESTORE_HANDLE}
    Set Suite Variable  ${SAFESTORE_HANDLE}  ${None}

Dump and Reset Logs
    Dump log  ${SAFESTORE_LOG_PATH}
    Remove File  ${SAFESTORE_LOG_PATH}*

Send signal to Safestore
    [Arguments]  ${signal}
    ${rc}  ${pid} =  Run And Return Rc And Output  pgrep safestore
    Evaluate  os.kill(${pid}, ${signal})  modules=os, signal

Wait Until Safestore Log Contains
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Safestore Log Contains  ${input}  timeout=${timeout}

*** Test Cases ***

Safestore is killed gracefully
    Start Safestore
    Send signal to Safestore  signal.SIGTERM
    Wait Until Safestore not running
    Safestore Log Contains  SafeStore received SIGTERM - shutting down
    Safestore Log Contains  Exiting SafeStore

Safestore exits on interrupt signal
    Start Safestore
    Send signal to Safestore  signal.SIGINT
    Wait Until Safestore not running
    Safestore Log Contains  SafeStore received SIGINT - shutting down
    Safestore Log Contains  Exiting SafeStore

SafeStore creates socket with correct permissions
    Start Safestore
    ${result} =  Run Process  ls  -l  ${SAFESTORE_SOCKET_PATH}
    Should Contain  ${result.stdout}  srw------- 1 sophos-spl-av root 0
    Stop Safestore