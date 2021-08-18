*** Settings ***
Suite Setup      Run Keywords
...              Regenerate Certificates  AND
...              Setup MCS Tests
Suite Teardown   Run Keywords
...              Uninstall SSPL Unless Cleanup Disabled  AND
...              Cleanup Certificates

Resource  ../mcs_router/McsRouterResources.robot


*** Keywords ***

