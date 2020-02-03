*** Settings ***
Documentation    Register SSPL with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  McsRouterNovaResources.robot

Library     ${libs_directory}/CentralUtils.py

Default Tags  CENTRAL  MCS

Suite Setup  Set Nova MCS CA Environment Variable
Suite Teardown  Set Nova MCS CA Environment Variable

Test Teardown  Nova Test Teardown  True

*** Test Cases ***

Successful Register With Central Overriding MCS Certificate
    Copy CA To File   /tmp/nova_mcs_rootca.crt  ${SOPHOS_INSTALL}/base/mcs/certs/
    Set MCS CA Environment Variable   ${SOPHOS_INSTALL}/base/mcs/certs/nova_mcs_rootca.crt
    File Should Not Exist   ${SOPHOS_INSTALL}/base/mcs/policy/MCS-25_policy.xml
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait New MCS Policy Downloaded

Successful Register With Central Replacing MCS Certificate
    Unset CA Environment Variable
    Copy File   /tmp/nova_mcs_rootca.crt  /tmp/mcs_rootca.crt.0
    Copy CA To File   /tmp/mcs_rootca.crt.0  ${SOPHOS_INSTALL}/base/mcs/certs/
    File Should Not Exist   ${SOPHOS_INSTALL}/base/mcs/policy/MCS-25_policy.xml
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait New MCS Policy Downloaded