*** Settings ***
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup      Run Keywords
...              Setup MCS Tests
Suite Teardown      Uninstall SSPL Unless Cleanup Disabled

*** Keywords ***

