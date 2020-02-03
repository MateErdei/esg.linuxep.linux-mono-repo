*** Settings ***
Documentation    Check MCS policy handling with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  McsRouterNovaResources.robot


Library    ${libs_directory}/MCSRouter.py
Library    ${libs_directory}/CentralUtils.py
Library     ${libs_directory}/LogUtils.py
# For proxies:
Library    ${libs_directory}/UpdateServer.py
Library    ${libs_directory}/ProxyUtils.py


## EXCLUDE_AWS because this requires the secureproxyserver
Default Tags  CENTRAL  MCS  EXCLUDE_AWS
Suite Setup     Restart Secure Server Proxy   # ensure the secureproxy is in a good state before tests
Test Teardown     Run Keywords
...               Remove Environment Variable    https_proxy   AND
...               Stop Proxy Servers                   AND
...               Stop Proxy If Running                AND
...               Nova Test Teardown  requireDeRegister=True   AND
...               Remove File   /tmp/cloud_tokens.txt   AND
...               Clean McsRouter Log File

*** Variables ***

${FAKE_PROXY_HOST}         NoProxyHere
${FAKE_PROXY_PORT}         10001
${SECURE_PROXY_HOST}       10.55.36.245
${SECURE_PROXY_PORT}       8888
${REAL_MR_HOST}            10.55.36.42
${REAL_MR_PORT}            8190
${PROXY_USER}              proxyuser
${PROXY_PASSWORD}          proxypassword
${MCS_ADDRESS}             mcs.sandbox.sophos:443

*** Test Cases ***

Register in Central through environment proxy
    [Documentation]  Derived from  CLOUD.PROXY.001_Register_in_cloud_through_environment_Proxy.sh
    Set Environment Variable  https_proxy   http://${SECURE_PROXY_HOST}:8888
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check MCS Router log contains proxy success  ${SECURE_PROXY_HOST}:8888


Register in central with localhost proxy
    [Documentation]  Derived from  CLOUD.PROXY.003_localhost_proxy.sh
    Start Simple Proxy Server    3333
    Set Environment Variable  https_proxy   http://localhost:3333
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check MCS Router log contains proxy success  localhost:3333


Register in cloud through pretend message relay
    [Documentation]  Derived from CLOUD.PROXY.006_Register_in_cloud_through_message_relay.sh
    ...              Using proxy server to pretend to be a message relay
    ${proxies} =  Set Variable  ${FAKE_PROXY_HOST}:${FAKE_PROXY_PORT},0,1;${SECURE_PROXY_HOST}:${SECURE_PROXY_PORT},1,2
    Log To Console  ${regCommand} --messagerelay ${proxies}
    Register With Central  ${regCommand} --messagerelay ${proxies}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MCS Router log contains message relay success  ${SECURE_PROXY_HOST}:${SECURE_PROXY_PORT}


Register in cloud through real message relay
    [Documentation]  Derived from CLOUD.PROXY.011_register_in_cloud_real_message_relay.sh
    ...              SavLinuxUC1 is on SavLinuxTrunkNet so not accessible to our test machines
    [Tags]  CENTRAL  MCS  EXCLUDE_AWS  MANUAL
    ${proxies} =  Set Variable  ${REAL_MR_HOST}:${REAL_MR_PORT},0,1
    Log To Console  ${regCommand} --messagerelay ${proxies}
    Register With Central  ${regCommand} --messagerelay ${proxies}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Check MCS Router log contains message relay success  ${REAL_MR_HOST}:${REAL_MR_PORT}


Register in cloud through basic auth proxy
    [Documentation]  Derived from CLOUD.PROXY.007_basic_auth.sh
    ${proxy_port} =  Set Variable  5003
    Require Proxy With Basic Authentication  ${proxy_port}
    Set Environment Variable  http_proxy  http://${PROXY_USER}:${PROXY_PASSWORD}@localhost:${proxy_port}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Check MCS Router log contains proxy success  localhost:${proxy_port}


Register in cloud through digest auth proxy
    [Documentation]  Derived from CLOUD.PROXY.009_digest_auth.sh
    ${proxy_port} =  Set Variable  5004
    start proxy server with digest auth  ${proxy_port}  ${PROXY_USER}  ${PROXY_PASSWORD}
    Set Environment Variable  http_proxy  http://${PROXY_USER}:${PROXY_PASSWORD}@localhost:${proxy_port}
    Register With Central  ${regCommand}
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Check MCS Router log contains proxy success  localhost:${proxy_port}

Register in cloud through pretend message relay in correct order
    [Documentation]  Derived from CLOUD.PROXY.008_Check_message_relay_ordering.sh
    ...              Using proxy server to pretend to be a message relay


    ${distance25} =  Calculate Proxy IP at distance  25
    ${distance9} =   Calculate Proxy IP at distance   9
    ${distance5} =   Calculate Proxy IP at distance   5
    ${distance1} =   Calculate Proxy IP at distance   1

    Modify Hosts File  FakeRelayTwentyFive=${distance25}  FakeRelayFive=${distance5}  FakeRelayOne=${distance1}  FakeRelayNine=${distance9}

    ${proxies} =  Set Variable  FakeRelayTwentyFive:6666,0,1;FakeRelayFive:6666,0,2;FakeRelayOne:6666,1,3;FakeRelayNine:6666,0,4;${SECURE_PROXY_HOST}:${SECURE_PROXY_PORT},1,5

    ${full_reg_command} =  Set Variable  ${regCommand} --messagerelay ${proxies}

    Log To Console  ${full_reg_command}
    Register With Central  ${full_reg_command}
    Wait For MCS Router To Be Running

    Wait Until Keyword Succeeds
    ...  120 seconds
    ...  5 secs
    ...  Check MCS Router log contains message relay success  ${SECURE_PROXY_HOST}:${SECURE_PROXY_PORT}

    Unmodify Hosts File  FakeRelayTwentyFive=${distance25}  FakeRelayFive=${distance5}  FakeRelayOne=${distance1}  FakeRelayNine=${distance9}

    Verify Message Relay Failure In Order  FakeRelayFive:6666  FakeRelayNine:6666  FakeRelayTwentyFive:6666  FakeRelayOne:6666




*** Keywords ***

Check MCS Router log contains message relay success
    [Arguments]    ${MESSAGE_RELAY}
    Check Mcsrouter Log Contains  Successfully connected to ${MCS_ADDRESS} via ${MESSAGE_RELAY}

Require Proxy With Basic Authentication
    [Arguments]    ${proxy_port}
    Start Proxy Server With Basic Auth  ${proxy_port}  ${PROXY_USER}  ${PROXY_PASSWORD}

Check MCS Router log contains proxy success
    [Arguments]    ${PROXY_HOST_PORT}
    Wait Until Keyword Succeeds
    ...  40 seconds
    ...  5 secs
    ...  Check Mcsrouter Log Contains  Successfully connected to ${MCS_ADDRESS} via ${PROXY_HOST_PORT}

Check MCS Router log contains message relay failure
    [Arguments]    ${MESSAGE_RELAY}
    Check Mcsrouter Log Contains  Failed connection with message relay via ${MESSAGE_RELAY} to ${MCS_ADDRESS}