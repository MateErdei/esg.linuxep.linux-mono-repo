*** Settings ***
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library   OperatingSystem

Resource  McsRouterResources.robot

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

Test Setup    Run Keyword and Ignore Error   Remove File   	${SOPHOS_INSTALL}/base/etc/mcs.config

*** Test Case ***
Fail Register with TLS1_2 Bad Certificate   
    Start HTTPS Server    --tls1_2
    # Use default CA file, which does not contain CA for local cloud
    Unset CA Environment Variable
    Fail Register With HTTPS Server Certificate Verify Failed

Test Register With TLS Below Minimum Accepted of TLSv1_2 Fails
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  EXCLUDE_UBUNTU20
    Start HTTPS Server    --tls1
    Fail Register With HTTPS Server   [SSL: UNSUPPORTED_PROTOCOL]
    
Fail Register If TLS1_1
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  EXCLUDE_UBUNTU20
    Start HTTPS Server    --tls1_1
    Fail Register With HTTPS Server   [SSL: UNSUPPORTED_PROTOCOL]

Fail Register If TLS1_1
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER
    Start HTTPS Server    --tls1_1
    Fail Register With HTTPS Server   [Errno 0]
Register with TLS1_2
    Start HTTPS Server    --tls1_2
    Set Local CA Environment Variable
    # Connection was successful in terms of TLS handshake
    Fail Register With HTTPS Server Gateway Error

Register With Server At Defualt Value PROTOCOL_TLS Will Resolve To TLSv1_2 Plus
    Start HTTPS Server   --tls
    Set Local CA Environment Variable
    # Connection was successful in terms of TLS handshake
    Fail Register With HTTPS Server Gateway Error

*** Keywords ***
Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}
