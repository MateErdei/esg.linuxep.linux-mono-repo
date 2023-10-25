*** Settings ***
Library     ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Test Teardown    MCSRouter Default Test Teardown

Default Tags  MCS  MCS_ROUTER  TAP_PARALLEL6

*** Test Case ***
Successful Start Up of MCS
    Start MCSRouter

    Wait Until Keyword Succeeds     #using this to ensure mcsrouter shutdown cleanly
    ...  10 secs
    ...  2 secs
    ...  check_mcsrouter_log_contains_pattern        (Plugin found: updatescheduler.json, with APPIDs: MCS, ALC|Plugin found: updatescheduler.json, with APPIDs: ALC, MCS)
    Check Mcsrouter Log Contains     Plugin found: mcsrouter.json, with APPIDs:
    Check Mcsrouter Log Contains     Plugin found: managementagent.json, with APPIDs:


