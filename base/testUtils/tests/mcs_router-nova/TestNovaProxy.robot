*** Settings ***
Documentation    Check MCS policy handling with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  McsRouterNovaResources.robot


Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/CentralUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
# For proxies:
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/ProxyUtils.py


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
${SECURE_PROXY_HOST}       ssplsecureproxyserver.eng.sophos
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

*** Keywords ***

Check MCS Router log contains message relay success
    [Arguments]    ${MESSAGE_RELAY}
    Check Mcsrouter Log Contains  Successfully connected to ${MCS_ADDRESS} via ${MESSAGE_RELAY}


Check MCS Router log contains proxy success
    [Arguments]    ${PROXY_HOST_PORT}
    Wait Until Keyword Succeeds
    ...  40 seconds
    ...  5 secs
    ...  Check Mcsrouter Log Contains  Successfully connected to ${MCS_ADDRESS} via ${PROXY_HOST_PORT}

Check MCS Router log contains message relay failure
    [Arguments]    ${MESSAGE_RELAY}
    Check Mcsrouter Log Contains  Failed connection with message relay via ${MESSAGE_RELAY} to ${MCS_ADDRESS}