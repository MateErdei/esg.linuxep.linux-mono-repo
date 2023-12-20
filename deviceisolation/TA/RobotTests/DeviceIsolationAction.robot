*** Settings ***
Resource        ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown

Force Tags    EXCLUDE_CENTOS7    EXCLUDE_RHEL79

*** Test Cases ***

Device Isolation Processes Enable Action
    Restart Device Isolation
    ${mark} =  Get Device Isolation Log Mark
    Should Exist   ${COMPONENT_ROOT_PATH}/bin/nft
    Remove File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    Send Enable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Enabling Device Isolation
    Wait For Log Contains From Mark    ${mark}    Device is now isolated
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    ${rules} =  Get File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    ${expected_rules} =  Get File    ${ROBOT_SCRIPTS_PATH}/policies/rules_with_no_exclusions.txt
    Should Be Equal As Strings  ${rules}  ${expected_rules}
    Send Disable Isolation Action  uuid=1

Device Isolation Processes Disable Action
    Restart Device Isolation
    ${mark} =  Get Device Isolation Log Mark
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is no longer isolated