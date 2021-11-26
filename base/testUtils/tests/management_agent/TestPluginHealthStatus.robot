*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py


Resource    ManagementAgentResources.robot

Default Tags    MANAGEMENT_AGENT

*** Test Cases ***
Verify Management Agent Can Check Good Plugin Health Status
    [Tags]  SMOKE  MANAGEMENT_AGENT  TAP_TESTS
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    ${SHS_STATUS_FILE} =  Set Variable  /opt/sophos-spl/base/mcs/status/SHS_status.xml

    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  File Should Exist   ${SHS_STATUS_FILE}

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    ${STATUS_CONTENT}=  Get File  ${SHS_STATUS_FILE}
    Log File  ${SHS_STATUS_FILE}
    Should Contain   ${STATUS_CONTENT}  ${EXPECTED_CONTENT}

    # clean up
    Stop Plugin
    Stop Management Agent

Verify Management Agent Can Check Bad Plugin Health Status
    [Tags]  SMOKE  MANAGEMENT_AGENT  TAP_TESTS
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    ${SHS_STATUS_FILE} =  Set Variable  /opt/sophos-spl/base/mcs/status/SHS_status.xml

    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  File Should Exist   ${SHS_STATUS_FILE}

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="3" /><item name="service" value="3" ><detail name="FakePlugin" value="1" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="3" ><detail name="FakePlugin" value="1" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    ${STATUS_CONTENT}=  Get File  ${SHS_STATUS_FILE}
    Log File  ${SHS_STATUS_FILE}
    Should Contain   ${STATUS_CONTENT}  ${EXPECTED_CONTENT}

    # clean up
    Stop Management Agent
