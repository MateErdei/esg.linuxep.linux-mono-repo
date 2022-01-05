*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
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
    ...  40
    ...  5
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
    ...  40
    ...  5
    ...  File Should Exist   ${SHS_STATUS_FILE}

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="3" /><item name="service" value="3" ><detail name="FakePlugin" value="2" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="3" ><detail name="FakePlugin" value="2" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    ${STATUS_CONTENT}=  Get File  ${SHS_STATUS_FILE}
    Log File  ${SHS_STATUS_FILE}
    Should Contain   ${STATUS_CONTENT}  ${EXPECTED_CONTENT}

    # clean up
    Stop Management Agent


Verify Management Agent does not check health when suldownloader is running
    [Tags]  MANAGEMENT_AGENT
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Create File   ${UPGRADING_MARKER_FILE}
    Setup Plugin Registry
    Start Management Agent

    ${SHS_STATUS_FILE} =  Set Variable  /opt/sophos-spl/base/mcs/status/SHS_status.xml

    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  check_management_agent_log_contains   Starting service health checks
    File Should Not Exist   ${SHS_STATUS_FILE}

    Remove File  ${UPGRADING_MARKER_FILE}

    Wait Until Keyword Succeeds
    ...  180
    ...  5
    ...  File Should Exist   ${SHS_STATUS_FILE}

    # clean up
    Stop Management Agent

Verify Log Behaviour When Plugin Health Retrieval Fails
    [Tags]    MANAGEMENT_AGENT
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent
    Start Plugin

    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  Check Management Agent Log Contains   Starting service health checks

    Stop Plugin
    Sleep  60s

    Check Management Agent Log Contains String N Times  Could not get health for service FakePlugin  1

    Start Plugin
    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  Check Management Agent Log Contains   Health restored for service FakePlugin

    Mark Management Agent Log
    Stop Plugin
    Sleep  60s

    Check Marked Management Agent Log Contains String N Times  Could not get health for service FakePlugin  1

    Start Plugin
    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  Check Management Agent Log Contains   Health restored for service FakePlugin

    # clean up
    Stop Plugin
    Stop Management Agent

*** Keywords ***
Service Sleep Teardown
    Revert Service  sophos-spl-update.service
    Terminate All Processes  kill=True
    General Test Teardown
