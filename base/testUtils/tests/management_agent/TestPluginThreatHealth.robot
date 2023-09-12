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
    Send Plugin Threat Health   ${content}

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Management Agent Log Contains  Running threat health task

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes

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
    Send Plugin Threat Health   ${content}

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Management Agent Log Contains  Running threat health task

    Stop Management Agent
    Wait For Threat Health Json to Contain  {"FakePlugin":{"displayName":"","healthType":4,"healthValue":2}}

    # Restart MA to force a re-write of the file on MA shutdown
    # It should have been read in on start and then re-saved as it on shutdown
    Start Management Agent
    Wait Until Keyword Succeeds
    ...  90 secs
    ...  5 secs
    ...  SHS Status File Contains  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="2" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="2" /></health>

    Stop Management Agent
    Wait For Threat Health Json to Contain  {"FakePlugin":{"displayName":"","healthType":4,"healthValue":2}}

*** Keywords ***
Wait For Threat Health Json to Contain
    [Arguments]   ${content_to_contain}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Threat Health Json Contains  ${content_to_contain}

Threat Health Json Contains
    [Arguments]   ${content_to_contain}
    ${threat_health_json} =  Get File  ${VAR_DIR}/sophosspl/ThreatHealth.json
    Log  ${threat_health_json}
    Should Be Equal As Strings  ${threat_health_json}  ${content_to_contain}