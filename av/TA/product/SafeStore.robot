*** Settings ***
Documentation   Product tests for SafeStore
Force Tags      PRODUCT  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Test Setup     SafeStore Test Setup
Test Teardown  SafeStore Test Teardown

*** Keywords ***

SafeStore Test Setup
    Component Test Setup

SafeStore Test Teardown
    Stop SafeStore
    List AV Plugin Path
    run teardown functions
    Check All Product Logs Do Not Contain Error
    Dump and Reset Logs
    Component Test TearDown

Start SafeStore
    ${handle} =  Start Process  ${SAFESTORE_BIN}  stdout=DEVNULL  stderr=DEVNULL
    Set Test Variable  ${SAFESTORE_HANDLE}  ${handle}
    Wait Until SafeStore running

Stop SafeStore
    ${result} =  Terminate Process  ${SAFESTORE_HANDLE}
    Set Suite Variable  ${SAFESTORE_HANDLE}  ${None}

Dump and Reset Logs
    Dump log  ${SAFESTORE_LOG_PATH}
    Remove File  ${SAFESTORE_LOG_PATH}*

Send signal to SafeStore
    [Arguments]  ${signal}
    ${rc}  ${pid} =  Run And Return Rc And Output  pgrep safestore
    Evaluate  os.kill(${pid}, ${signal})  modules=os, signal

Wait Until SafeStore Log Contains
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  SafeStore Log Contains  ${input}  timeout=${timeout}

*** Test Cases ***

SafeStore is killed gracefully
    Start SafeStore
    Send signal to SafeStore  signal.SIGTERM
    Wait Until SafeStore not running
    SafeStore Log Contains  SafeStore received SIGTERM - shutting down
    SafeStore Log Contains  Exiting SafeStore

SafeStore exits on interrupt signal
    Start SafeStore
    Send signal to SafeStore  signal.SIGINT
    Wait Until SafeStore not running
    SafeStore Log Contains  SafeStore received SIGINT - shutting down
    SafeStore Log Contains  Exiting SafeStore

SafeStore creates socket with correct permissions
    Start SafeStore
    ${result} =  Run Process  ls  -l  ${SAFESTORE_SOCKET_PATH}
    # the second variant is needed for Centos (where alternate access methods are enabled)
    Should Contain Any  ${result.stdout}
    ...  srw------- 1 sophos-spl-av root 0
    ...  srw-------. 1 sophos-spl-av root 0
    Stop SafeStore