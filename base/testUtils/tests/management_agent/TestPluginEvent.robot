*** Settings ***
Documentation     Test the Management Agent

Test Teardown     Plugin Event Test Teardown

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py

Resource    ManagementAgentResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags   MANAGEMENT_AGENT


*** Test Cases ***
Verify Management Agent Creates New Event File When Plugin Raises A New Event
    [Tags]  SMOKE  MANAGEMENT_AGENT  TAP_TESTS
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


*** Keywords ***

Plugin Event Test Teardown
    General Test Teardown
    Stop Plugin
    Stop Management Agent
    Terminate All Processes  kill=True

Management Agent Log Contains Error N Times
    [Arguments]  ${errorMessage}  ${expectedTimes}
    ${managementLog} =  Get File  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    Should Contain X Times  ${managementLog}  ${errorMessage}  ${expectedTimes}  Management agent should contain : "${errorMessage}" only ${expectedTimes} time/s