*** Settings ***
Documentation    Test SSPL with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  McsRouterNovaResources.robot

Library     ${LIBS_DIRECTORY}/CentralUtils.py

Default Tags  CENTRAL  MCS  MANAGEMENT_AGENT

Test Teardown     Nova Test Teardown  requireDeRegister=True

*** Test Cases ***
Management Agent Runs With Low Privilege User
    [Documentation]  Derived from CLOUD.009_lower_privs.sh
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    ${pid} =     Run Process    pgrep    -f      sophos_managementagent
    log   ${pid.stdout}
    ${result} =     Run Process    ps    -o    user   -p   ${pid.stdout}
    Should Contain  ${result.stdout}   sophos-spl-user
    ${result} =     Run Process    ps    -o    group   -p   ${pid.stdout}
    Should Contain  ${result.stdout}   sophos-spl-group