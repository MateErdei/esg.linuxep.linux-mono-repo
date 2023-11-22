*** Settings ***
Resource        ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot

Library         ${COMMON_TEST_LIBS}/ActionUtils.py
Library         ${COMMON_TEST_LIBS}/FakeMCS.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown

*** Test Cases ***

Device Isolation Logs Valid Enable Action
    ${mark} =  Get Device Isolation Log Mark
    Send Enable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Enabling Device Isolation

Device Isolation Logs Valid Disable Action
    ${mark} =  Get Device Isolation Log Mark
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation

*** Keywords ***

Send Enable Isolation Action
    [Arguments]  ${uuid}
    Send Isolation Action  enable_isolation.xml  ${uuid}

Send Disable Isolation Action
    [Arguments]  ${uuid}
    Send Isolation Action  disable_isolation.xml  ${uuid}

Send Isolation Action
    [Arguments]  ${filename}  ${uuid}
    ${creation_time_and_ttl} =  get_valid_creation_time_and_ttl
    ${actionFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/SHS_action_${creation_time_and_ttl}.xml
    ${srcFileName} =  Set Variable  ${ROBOT_SCRIPTS_PATH}/actions/${filename}
    send_action  ${srcFileName}  ${actionFileName}  UUID=${uuid}
