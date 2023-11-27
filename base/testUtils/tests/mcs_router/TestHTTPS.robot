*** Settings ***
Library     ${COMMON_TEST_LIBS}/CentralUtils.py
Library   OperatingSystem

Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Force Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_PARALLEL4

Test Setup    Run Keyword and Ignore Error   Remove File   	${SOPHOS_INSTALL}/base/etc/mcs.config

*** Test Case ***
Fail Register with TLS1_2 Bad Certificate   
    Start HTTPS Server    --tls1_2
    # Use default CA file, which does not contain CA for local cloud
    Unset CA Environment Variable
    Fail Register With HTTPS Server Certificate Verify Failed

Test Register With TLS Below Minimum Accepted of TLSv1_2 Fails
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  EXCLUDE_UBUNTU2004  EXCLUDE_UBUNTU2204  EXCLUDE_RHEL9
    Start HTTPS Server    --tls1
    Fail Register With HTTPS Server   [SSL: SSLV3_ALERT_HANDSHAKE_FAILURE]
    
Fail Register If TLS1_1
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  EXCLUDE_UBUNTU2004  EXCLUDE_UBUNTU2204  EXCLUDE_RHEL9
    Start HTTPS Server    --tls1_1
    Fail Register With HTTPS Server   [SSL: SSLV3_ALERT_HANDSHAKE_FAILURE]


*** Keywords ***
Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}
