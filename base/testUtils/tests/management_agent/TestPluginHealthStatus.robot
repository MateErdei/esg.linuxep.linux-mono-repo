*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py

Resource  ../mcs_router/McsRouterResources.robot
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
    ${SHS_POLICY_FILE} =  Set Variable  /opt/sophos-spl/base/mcs/internal_policy/internal_EPHEALTH.json

    Wait Until Keyword Succeeds
    ...  15
    ...  5
    ...  Check Management Agent Log Contains   Management Agent running.
    File Should Exist   ${SHS_POLICY_FILE}
    File Should Not Exist   ${SHS_STATUS_FILE}
    #check overallHealth is good before health status is calculated
    ${EXPECTEDPOLICY_CONTENT}=  Set Variable  {"health":1,"service":1,"threat":1,"threatService":1}
    File Should Contain   ${SHS_POLICY_FILE}  ${EXPECTEDPOLICY_CONTENT}
    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  File Should Exist   ${SHS_STATUS_FILE}

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    Wait Until Keyword Succeeds
    ...  5
    ...  1
    ...  File Should Contain   ${SHS_STATUS_FILE}  ${EXPECTED_CONTENT}


    # clean up
    Stop Plugin
    Stop Management Agent

Verify Management Agent Can Receive Service Health Information
    [Tags]  SMOKE  MANAGEMENT_AGENT  TAP_TESTS
        # make sure no previous status xml file exists.
        Remove Status Xml Files

        Setup Plugin Registry
        Start Management Agent

        Start Plugin
        Set Fake Plugin App Id    HBT
        Set Service Health    0    True    fake-utm-id-007

        ${SHS_STATUS_FILE} =  Set Variable  /opt/sophos-spl/base/mcs/status/SHS_status.xml
        ${SHS_POLICY_FILE} =  Set Variable  /opt/sophos-spl/base/mcs/internal_policy/internal_EPHEALTH.json

        Wait Until Keyword Succeeds
        ...  40
        ...  5
        ...  Check Management Agent Log Contains   Starting service health checks

        Wait Until Keyword Succeeds
        ...  180
        ...  5
        ...  File Should Exist   ${SHS_STATUS_FILE}

        ${EXPECTEDPOLICY_CONTENT}=  Set Variable  {"health":1,"service":1,"threat":1,"threatService":1}

        Wait Until Keyword Succeeds
        ...  180
        ...  5
        ...  File Should Contain   ${SHS_POLICY_FILE}  ${EXPECTEDPOLICY_CONTENT}

        Wait Until Keyword Succeeds
        ...  40
        ...  5
        ...  File Should Exist   ${SHS_STATUS_FILE}

        ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="true" activeHeartbeatUtmId="fake-utm-id-007"><item name="health" value="1" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

        Wait Until Keyword Succeeds
        ...  40
        ...  5
        ...  File Should Contain   ${SHS_STATUS_FILE}  ${EXPECTED_CONTENT}


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
    ${SHS_POLICY_FILE} =  Set Variable  /opt/sophos-spl/base/mcs/internal_policy/internal_EPHEALTH.json

    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  File Should Exist   ${SHS_STATUS_FILE}

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="3" /><item name="service" value="3" ><detail name="FakePlugin" value="2" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="3" ><detail name="FakePlugin" value="2" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>


    Log File  ${SHS_STATUS_FILE}
    Wait Until Keyword Succeeds
    ...  5
    ...  1
    ...  File Should Contain   ${SHS_STATUS_FILE}  ${EXPECTED_CONTENT}

    ${EXPECTEDPOLICY_CONTENT}=  Set Variable  {"health":3,"service":3,"threat":1,"threatService":3}

    Wait Until Keyword Succeeds
    ...  5
    ...  1
    ...  File Should Contain   ${SHS_POLICY_FILE}  ${EXPECTEDPOLICY_CONTENT}
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
