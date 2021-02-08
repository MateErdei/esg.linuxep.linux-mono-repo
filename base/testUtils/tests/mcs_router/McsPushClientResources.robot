*** Settings ***

Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py


Library     String
Resource    McsRouterResources.robot
Resource    ../upgrade_product/UpgradeResources.robot

*** Variables ***
${MCS_ROUTER_LOG}   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

*** Keywords ***
Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    [Arguments]  ${proxy}=no
    Run Keyword If  '${proxy}' != 'proxy'
    ...  Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct  ELSE
    ...  Push Client started and connects to Push Server when the MCS Client receives MCS Policy Proxy

Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct
    Wait Until Keyword Succeeds
    ...          10s
    ...          1s
    ...          Run Keywords
    ...          Check Mcsrouter Log Contains   Push Server settings changed. Applying it    AND
    ...          Check Mcsrouter Log Contains   Push client successfully connected to localhost:4443 directly

Push Client started and connects to Push Server when the MCS Client receives MCS Policy Proxy
    Wait Until Keyword Succeeds
    ...          10s
    ...          1s
    ...          Run Keywords
    ...          Check Mcsrouter Log Contains   Push Server settings changed. Applying it    AND
    ...          Check Mcsrouter Log Contains   Push client successfully connected to localhost:4443 via localhost:1235

Send Message To Push Server And Expect It In MCSRouter Log
    [Arguments]  ${message}
    Send Message To Push Server    ${message}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command: ${message}

Check Connected To Fake Cloud
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains     Successfully directly connected to localhost:4443

Push Client Test Teardown
    Push Server Teardown with MCS Fake Server
    Stop Proxy Servers
    Stop Mcsrouter If Running