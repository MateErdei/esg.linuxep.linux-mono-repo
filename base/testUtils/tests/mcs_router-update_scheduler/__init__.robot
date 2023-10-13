*** Settings ***
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup      Run Keywords
...              Regenerate Certificates  AND
...              Setup MCS Tests
Suite Teardown   Run Keywords
...              Uninstall SSPL Unless Cleanup Disabled  AND
...              Cleanup Certificates

*** Keywords ***

