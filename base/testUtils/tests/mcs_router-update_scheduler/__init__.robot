*** Settings ***
Suite Setup      Run Keywords
...              Setup MCS Tests  AND
...              Regenerate Certificates
Suite Teardown   Run Keywords
...              Uninstall SSPL Unless Cleanup Disabled  AND
...              Cleanup Certificates

Resource  ../mcs_router/McsRouterResources.robot


*** Keywords ***

