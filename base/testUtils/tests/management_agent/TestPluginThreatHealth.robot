*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py

Resource    ManagementAgentResources.robot

Default Tags    MANAGEMENT_AGENT  TAP_TESTS

*** Test Cases ***
Management Agent Can Receive Plugin Threat Health And It Updates
    Setup Plugin Registry
    Start Management Agent
    Start Plugin

    ${content}    Evaluate    str('{"ThreatHealth": 99}')
    send_plugin_threat_health   ${content}

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Management Agent Log Contains  Running threat health task

    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Should Exist  ${MCS_DIR}/status/SHS_status.xml

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains   <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="99" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="99" /></health>

    ${content}    Evaluate    str('{"ThreatHealth": 1}')
    send_plugin_threat_health   ${content}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains   <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>


Management Agent Persists Threat Health Of Plugins
    Setup Plugin Registry
    Start Management Agent
    Start Plugin

    ${content}    Evaluate    str('{"ThreatHealth": 2}')
    send_plugin_threat_health   ${content}

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Management Agent Log Contains  Running threat health task

#    Stop Plugin
    Stop Management Agent
    ${threatHealthJsonString} =  Get File  ${VAR_DIR}/sophosspl/ThreatHealth.json
    Log  ${threatHealthJsonString}
    Should Be Equal As Strings  ${threatHealthJsonString}  {"FakePlugin":{"displayName":"","healthType":4,"healthValue":2}}

    # Restart MA to force a re-write of the file on MA shutdown
    # It should have been read in on start and then re-saved as it on shutdown
    Start Management Agent
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="2" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="2" /></health>

    Stop Management Agent
    ${threatHealthJsonString} =  Get File  ${VAR_DIR}/sophosspl/ThreatHealth.json
    Log  ${threatHealthJsonString}
    Should Be Equal As Strings  ${threatHealthJsonString}  {"FakePlugin":{"displayName":"","healthType":4,"healthValue":2}}


*** Keywords ***
SHS Status File Contains
    [Arguments]  ${content_to_contain}
    ${shsStatus} =  Get File   ${MCS_DIR}/status/SHS_status.xml
    Log  ${shsStatus}
    Should Contain  ${shsStatus}  ${content_to_contain}
