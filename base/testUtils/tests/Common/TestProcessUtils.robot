*** Settings ***
Resource  ../installer/InstallerResources.robot

Library  ${LIBS_DIRECTORY}/ProcessUtils.py
Library  ${LIBS_DIRECTORY}/LogUtils.py

Test Teardown  Test Teardown
Suite Setup  Suite Setup

Default Tags  TAP_TESTS
*** Keywords ***
Test Teardown
    General Test Teardown

Suite Teardown
    Require Uninstalled

Suite Setup
    Require Installed

Check Process Not Running
    [Arguments]  ${pid}
    # kill -0 checks if process is alive
    ${r} =  Run Process  kill  -0  ${pid}
    # rc 1 means no process is alive with that pid
    Should Be Equal As Strings  ${r.rc}  1

*** Test Cases ***

Test Proc Utilities Ensures No Processes Running With Command Line Kills Single Process
    ${pid}  ${name}  ${path} =  spawn_sleep_process
    ${r} =  Run Process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/TestProcessUtils  ${name}  ${path}
    log  ${r.stdout}
    log  ${r.stderr}
    Should Contain  ${r.stderr}  Stopping process (${name}) pid: ${pid}
    Should Contain N Times  ${r.stderr}  Stopping process  1
    Check Process Not Running  ${pid}


Test Proc Utilities Ensures No Processes Running With Command Line Kills Multiple Processes
    ${pid1}  ${name}  ${path} =  spawn_sleep_process
    ${pid2}  ${name}  ${path} =  spawn_sleep_process
    ${pid3}  ${name}  ${path} =  spawn_sleep_process
    ${r} =  Run Process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/TestProcessUtils  ${name}  ${path}
    Check Process Not Running  ${pid1}
    Check Process Not Running  ${pid2}
    Check Process Not Running  ${pid3}
    log  ${r.stdout}
    log  ${r.stderr}
    Should Contain  ${r.stderr}  Stopping process (${name}) pid: ${pid1}
    Should Contain  ${r.stderr}  Stopping process (${name}) pid: ${pid2}
    Should Contain  ${r.stderr}  Stopping process (${name}) pid: ${pid3}
    Should Contain N Times  ${r.stderr}  Stopping process  3


Test Proc Utilities Can Repeatedly Kill The Same Process
    ${pid1}  ${name}  ${path} =  spawn_sleep_process
    ${r} =  Run Process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/TestProcessUtils  ${name}  ${path}
    Should Contain  ${r.stderr}  Stopping process (${name}) pid: ${pid1}
    Should Contain N Times  ${r.stderr}  Stopping process  1
    Check Process Not Running  ${pid1}

    ${pid2}  ${name}  ${path} =  spawn_sleep_process
    ${r} =  Run Process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/TestProcessUtils  ${name}  ${path}
    Should Contain  ${r.stderr}  Stopping process (${name}) pid: ${pid2}
    log  ${r.stderr}
    log  ${pid1}
    Should Not Contain  ${r.stderr}  ${pid1.__str__()}
    Should Contain N Times  ${r.stderr}  Stopping process  1
    Check Process Not Running  ${pid2}

    ${pid3}  ${name}  ${path} =  spawn_sleep_process
    ${r} =  Run Process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/TestProcessUtils  ${name}  ${path}
    Should Contain  ${r.stderr}  Stopping process (${name}) pid: ${pid3}
    Should Contain N Times  ${r.stderr}  Stopping process  1
    Should Not Contain  ${r.stderr}  ${pid1.__str__()}
    Should Not Contain  ${r.stderr}  ${pid2.__str__()}
    Check Process Not Running  ${pid3}


Test Proc Utilities Handles Trying To Kill Non Existent Process
    ${r} =  Run Process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/TestProcessUtils  ThisProcessDoesNotExist  /bin/ThisProcessDoesNotExist
    Should Be Equal As Strings  ${r.rc}  0
    log  ${r.stdout}
    log  ${r.stderr}
    Should Contain  ${r.stderr}  Checking currently running executables that contain comm: ThisProcessDoesNotExist
    Should Not Contain  ${r.stderr}  Stopping Process