*** Settings ***
Documentation     Test the Management Agent

Test Teardown     Plugin Event Test Teardown

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${COMMON_TEST_LIBS}/ActionUtils.py
Library     ${COMMON_TEST_LIBS}/EventUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/OnFail.py
Library     ${COMMON_TEST_LIBS}/FakePluginWrapper.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot

Force Tags     MANAGEMENT_AGENT    TAP_PARALLEL1

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

Verify Management Agent Goes Into Outbreak Mode And Out After Clear Action
    [Tags]  SMOKE
    # make sure no previous event xml file exists.
    Remove Event Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin
    set_fake_plugin_app_id  CORE

    ${eventContent} =  Get File  ${SUPPORT_FILES}/CORE_events/detection.xml
    Enter Outbreak Mode  ${eventContent}

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


*** Keywords ***
Plugin Event Test Teardown
    run teardown functions
    Run Keyword If Test Failed     Log File   /tmp/fake_plugin.log
    Run Keyword And Ignore Error   Remove File   /tmp/fake_plugin.log
    General Test Teardown
    Stop Plugin
    Stop Management Agent
    Terminate All Processes  kill=True

Management Agent Log Contains Error N Times
    [Arguments]  ${errorMessage}  ${expectedTimes}
    ${managementLog} =  Get File  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    Should Contain X Times  ${managementLog}  ${errorMessage}  ${expectedTimes}  Management agent should contain : "${errorMessage}" only ${expectedTimes} time/s



