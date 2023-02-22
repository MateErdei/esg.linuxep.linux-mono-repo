*** Settings ***
Documentation     Test the Management Agent

Test Teardown     Plugin Event Test Teardown

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/EventUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/OnFail.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py

Resource    ManagementAgentResources.robot
Resource  ../GeneralTeardownResource.robot

Force Tags     MANAGEMENT_AGENT

*** Test Cases ***
Verify Management Agent Creates New Event File When Plugin Raises A New Event
    # make sure no previous event xml file exists.
    Remove Event Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    ${eventContent} =     Evaluate    str('<a>event1</a>')
    Send Plugin Event   ${eventContent}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Event File     ${eventContent}

Verify Management Agent Goes Into Outbreak Mode After 100 Events
    [Tags]   MANAGEMENT_AGENT
    # make sure no previous event xml file exists.
    Remove Event Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin
    set_fake_plugin_app_id  CORE

    ${eventContent} =  Get File  ${SUPPORT_FILES}/CORE_events/detection.xml

    ${mark} =  mark_log_size  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    register on fail  dump_marked_log  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log  ${mark}

    Repeat Keyword  110 times  Send Plugin Event   ${eventContent}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Event File     ${eventContent}

    wait for log contains from mark  ${mark}  Entering outbreak mode: Further detections will not be reported to Central

    # Check we have the outbreak event
    check at least one event has substr  ${SOPHOS_INSTALL}/base/mcs/event  sophos.core.outbreak

    # count events
    ${count} =  Count Files in Directory  ${SOPHOS_INSTALL}/base/mcs/event
    Should be equal as Integers  ${count}  101

Verify Management Agent Goes Into Outbreak Mode And Out After Clear Action
    [Tags]  SMOKE  MANAGEMENT_AGENT  TAP_TESTS
    # make sure no previous event xml file exists.
    Remove Event Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin
    set_fake_plugin_app_id  CORE

    ${eventContent} =  Get File  ${SUPPORT_FILES}/CORE_events/detection.xml

    ${mark} =  mark_log_size  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    register on fail  dump_marked_log  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log  ${mark}

    Repeat Keyword  110 times  Send Plugin Event   ${eventContent}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Event File     ${eventContent}

    wait for log contains from mark  ${mark}  Entering outbreak mode: Further detections will not be reported to Central

    # Check we have the outbreak event
    check at least one event has substr  ${SOPHOS_INSTALL}/base/mcs/event  sophos.core.outbreak

    # count events
    ${count} =  Count Files in Directory  ${SOPHOS_INSTALL}/base/mcs/event
    Should be equal as Integers  ${count}  101

    Remove Event Xml Files
    ${mark} =  mark_log_size  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    ${uuid} =  Set Variable  5df69683-a5a2-5d96-897d-06f9c4c8c7bf
    Send Clear Action  ${uuid}
    wait for log contains from mark  ${mark}  Leaving outbreak mode

    Send Plugin Event   ${eventContent}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Event File     ${eventContent}


Verify Sending Bad Message On Management Agent Socket Does Not Stop Plugin Registering Or Working
    ${errorMessage} =  Set Variable  reactor <> Reactor: callback process failed with message: Bad formed message: Protobuf parse error

    # make sure no previous event xml file exists.
    Remove Event Xml Files

    Setup Plugin Registry
    Start Management Agent

    Send Message To Management Agent Without Protobuf Serialisation  Bad Message 1

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Management Agent Log Contains Error N Times  ${errorMessage}  1

    Start Plugin

    Send Message To Management Agent Without Protobuf Serialisation  Bad Message 2

    ${eventContent} =     Evaluate    str('<a>event1</a>')
    Send Plugin Event   ${eventContent}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Event File     ${eventContent}

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Management Agent Log Contains Error N Times  ${errorMessage}  2


*** Keywords ***

Plugin Event Test Teardown
    run teardown functions
    General Test Teardown
    Stop Plugin
    Stop Management Agent
    Terminate All Processes  kill=True

Management Agent Log Contains Error N Times
    [Arguments]  ${errorMessage}  ${expectedTimes}
    ${managementLog} =  Get File  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    Should Contain X Times  ${managementLog}  ${errorMessage}  ${expectedTimes}  Management agent should contain : "${errorMessage}" only ${expectedTimes} time/s


Send Clear Action
    [Arguments]  ${uuid}

    ${creation_time_and_ttl} =  get_valid_creation_time_and_ttl
    ${actionFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/CORE_action_${creation_time_and_ttl}.xml
    ${srcFileName} =  Set Variable  ${SUPPORT_FILES}/CORE_actions/clear.xml
    send_action  ${srcFileName}  ${actionFileName}  UUID=${uuid}
