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
MCS Router Does Not Crash When MCS DEBUG Environment Variable Is Set
    Log To Console  ${regCommand}
    Set Environment Variable    MCS_DEBUG   True
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    File Should Not Exist  ${SOPHOS_INSTALL}/base/etc/mcsrouter.conf

Mcsrouter reports Timeout When Unable To Contact Central due to Drop Rule
    [Documentation]  Derived from CLOUD.ERROR.005_timeout_after_internet_connectivity_lost.sh
    [Tags]  CENTRAL   MCS  MANAGEMENT_AGENT  MANUAL
    [Timeout]  10 minutes
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait New MCS Policy Downloaded
    Run Process   iptables   -A   INPUT   -s   ${CLOUD_IP}   -j   DROP

    Check MCS Router Running
    Check for timeout


Successful Register With Central
    [Documentation]  Derived from CLOUD.001_Register_in_cloud.sh
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud


Successful Register With Central And Correct Status Is Sent Up
    [Documentation]  Derived from CLOUD.001_Register_in_cloud.sh
    Log To Console  ${regCommand}
    Create File  ${SOPHOS_INSTALL}/base/mcs/policy/YUK.xml
    Register With Central  ${regCommand}

    Check Register Central Log Contains In Order   <productType>sspl</productType>  <platform>linux</platform>  <isServer>1</isServer>

    Check Register Central Log Contains In Order  <policy-status latest=  <policy app="YUK" latest=  </policy-status>

Successful Register With Central And Correct Status Is Sent Up In Amazon
    [Documentation]  Derived from CLOUD.001_Register_in_cloud.sh
    [Tags]  AMAZON_LINUX  CENTRAL  MCS
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}

    Check Register Central Log Contains In Order   <aws>  <region>  </region>  <accountId>  </accountId>  <instanceId>  </instanceId>  </aws>

Register Fails with bad token against Central
    [Documentation]  Derived from CLOUD.ERROR.003_Bad_token.sh
    Fail Register With Central With Bad Token

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check MCS Router Not Running

Successful Re-Register With Central
    [Documentation]  Derived from CLOUD.003_reregister_in_cloud.sh
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Delete Server From Cloud
    ${retcode}=    Check Server In Cloud
    Should Be Equal As Integers   ${retcode}     1    Server is still in cloud
    Register With Central  ${regCommand}
    Wait For Server In Cloud
