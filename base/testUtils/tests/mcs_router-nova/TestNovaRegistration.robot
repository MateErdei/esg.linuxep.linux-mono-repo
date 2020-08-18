*** Settings ***
Documentation    Register SSPL with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  McsRouterNovaResources.robot

Library     ${LIBS_DIRECTORY}/CentralUtils.py

Default Tags  CENTRAL  MCS

Test Teardown     Run Keywords
...               Remove Environment Variable    MCS_DEBUG    AND
...               Run Process   iptables   -D   INPUT   -s   ${CLOUD_IP}   -j   DROP    AND
...               Run Process   iptables   -D   INPUT   -s   ${CLOUD_IP}   -j   REJECT  AND
...               Nova Test Teardown  requireDeRegister=True   AND
...               Remove File    ${SOPHOS_INSTALL}/logs/base/register_central.log

*** Test Cases ***
Mcsrouter reports Timeout When Unable To Contact Central due to Reject Rule
    [Documentation]  Derived from CLOUD.ERROR.005_timeout_after_internet_connectivity_lost.sh
    [Tags]  CENTRAL   MCS  MANAGEMENT_AGENT
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait New MCS Policy Downloaded
    # in order to effectively reject the response from cloud to the end point
    # this rule has to be the first in the INPUT category
    Run Process   iptables   -I   INPUT  1   -s   ${CLOUD_IP}   -j   REJECT

    Check MCS Router Running
    Check for timeout
    Check Mcsrouter Log Contains  Got socket error
    Check Mcsrouter Log Does Not Contain   Traceback

Successful Register With Central And Correct Status Is Sent Up In Amazon
    [Documentation]  Derived from CLOUD.001_Register_in_cloud.sh
    [Tags]  AMAZON_LINUX  CENTRAL  MCS
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}

    Check Register Central Log Contains In Order   <aws>  <region>  </region>  <accountId>  </accountId>  <instanceId>  </instanceId>  </aws>

