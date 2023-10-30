*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/FakePluginWrapper.py

Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Force Tags     MANAGEMENT_AGENT  TEST_PLUGIN_HEALTH_STATUS  TAP_PARALLEL1

*** Test Cases ***
Verify Management Agent Can Check Good Plugin Health Status
    [Tags]  SMOKE
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    # Clears the management agent log, so can't use marks
    Start Management Agent

    Start Plugin

    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG}   Management Agent running.  timeout=${15}

    File Should Exist   ${SHS_POLICY_FILE}
    File Should Not Exist   ${SHS_STATUS_FILE}
    #check overallHealth is good before health status is calculated
    ${EXPECTEDPOLICY_CONTENT}=  Set Variable  {"health":1,"service":1,"threat":1,"threatService":1}
    File Should Contain   ${SHS_POLICY_FILE}  ${EXPECTEDPOLICY_CONTENT}
    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    Wait Until Keyword Succeeds
    ...  5
    ...  1
    ...  File Should Contain   ${SHS_STATUS_FILE}  ${EXPECTED_CONTENT}

    # clean up
    Stop Plugin
    Stop Management Agent


Verify Management Agent Can Receive Service Health Information
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry

    # Clears the management agent log:
    Start Management Agent

    Start Plugin
    Set Fake Plugin App Id    HBT
    Set Service Health    0    True    fake-utm-id-007

    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG}  Starting service health checks  timeout=${120}

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes

    ${EXPECTEDPOLICY_CONTENT}=  Set Variable  {"health":1,"service":1,"threat":1,"threatService":1}

    Wait Until Keyword Succeeds
    ...  180
    ...  5
    ...  File Should Contain   ${SHS_POLICY_FILE}  ${EXPECTEDPOLICY_CONTENT}

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="true" activeHeartbeatUtmId="fake-utm-id-007"><item name="health" value="1" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  File Should Contain   ${SHS_STATUS_FILE}  ${EXPECTED_CONTENT}

    # clean up
    Stop Plugin
    Stop Management Agent


Verify Management Agent Can Check Bad Plugin Health Status
    [Tags]  SMOKE
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes

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
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Create File   ${UPGRADING_MARKER_FILE}
    Setup Plugin Registry
    Start Management Agent

    # Management agent won't ever start monitoring health until the upgrading marker goes away
    Sleep  ${10}

    File Should Not Exist   ${SHS_STATUS_FILE}

    Remove File  ${UPGRADING_MARKER_FILE}

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=3 minutes

    # clean up
    Stop Management Agent


Verify Management Agent Does Not Report Health Of Removed Plugins
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin
    Set Fake Plugin App Id    HBT
    Set Service Health    0    True    fake-utm-id-007

    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG}  Starting service health checks  timeout=${120}

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes

    ${EXPECTEDPOLICY_CONTENT}=  Set Variable  {"health":1,"service":1,"threat":1,"threatService":1}

    Wait Until Keyword Succeeds
    ...  180
    ...  5
    ...  File Should Contain   ${SHS_POLICY_FILE}  ${EXPECTEDPOLICY_CONTENT}

    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="true" activeHeartbeatUtmId="fake-utm-id-007"><item name="health" value="1" /><item name="service" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="FakePlugin" value="0" /><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  File Should Contain   ${SHS_STATUS_FILE}  ${EXPECTED_CONTENT}

    # Get rid of plugin
    Stop Plugin
    ${ma_mark} =  Mark Log Size    ${MANAGEMENT_AGENT_LOG}
    Remove Fake Plugin From Registry

    Wait For Log Contains From Mark    ${ma_mark}    FakePlugin has been uninstalled.  timeout=${180}

    Wait Until Keyword Succeeds
    ...  180
    ...  5
    ...  File Should Contain   ${SHS_POLICY_FILE}  ${EXPECTEDPOLICY_CONTENT}

    ${EXPECTED_CONTENT}=  Set Variable  <?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="service" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>

    Wait Until Keyword Succeeds
    ...  40
    ...  5
    ...  File Should Contain   ${SHS_STATUS_FILE}  ${EXPECTED_CONTENT}

    Stop Management Agent


Verify Log Behaviour When Plugin Health Retrieval Fails
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent
    Start Plugin

    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG}  Starting service health checks  timeout=${120}

    Stop Plugin
    Sleep  60s

    Check Management Agent Log Contains String N Times  Could not get health for service FakePlugin  1

    ${ma_mark} =  Mark Log Size    ${MANAGEMENT_AGENT_LOG}
    Start Plugin
    Wait For Log Contains From Mark    ${ma_mark}    Health restored for service FakePlugin  timeout=${40}

    Mark Management Agent Log
    Stop Plugin
    Sleep  60s

    Check Marked Management Agent Log Contains String N Times  Could not get health for service FakePlugin  1

    ${ma_mark} =  Mark Log Size    ${MANAGEMENT_AGENT_LOG}
    Start Plugin
    Wait For Log Contains From Mark    ${ma_mark}    Health restored for service FakePlugin  timeout=${40}

    # clean up
    Stop Plugin
    Stop Management Agent


*** Variables ***
${MANAGEMENT_AGENT_LOG}         ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log

*** Keywords ***
Service Sleep Teardown
    Revert Service  sophos-spl-update.service
    Terminate All Processes  kill=True
    General Test Teardown
