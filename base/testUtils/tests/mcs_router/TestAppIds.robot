*** Settings ***
Resource  McsRouterResources.robot

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Test Teardown    MCSRouter Default Test Teardown

Default Tags  MCS  MCS_ROUTER

*** Test Case ***
Successful Start Up of MCS
    Start MCSRouter

    Wait Until Keyword Succeeds     #using this to ensure mcsrouter shutdown cleanly
    ...  10 secs
    ...  2 secs
    ...  Check Mcsrouter Log Contains     Plugin found: updatescheduler.json, with APPIDs: ALC
    Check Mcsrouter Log Contains     Plugin found: mcsrouter.json, with APPIDs:
    Check Mcsrouter Log Contains     Plugin found: managementagent.json, with APPIDs:


