*** Settings ***
Documentation    Check MCS policy handling with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  McsRouterNovaResources.robot

Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     ${libs_directory}/LogUtils.py

Default Tags  CENTRAL  MCS

Test Teardown     Nova Test Teardown  requireDeRegister=True

*** Test Cases ***
Check Central Policy
    [Documentation]  Derived from CLOUD.MCS.001_Check_policy.sh
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait New MCS Policy Downloaded
