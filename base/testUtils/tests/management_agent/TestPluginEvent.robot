*** Settings ***
Documentation     Test the Management Agent

Test Teardown     Plugin Event Test Teardown

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/FakePluginWrapper.py

Resource    ManagementAgentResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags   MANAGEMENT_AGENT

*** Test Cases ***
Verify Management Agent Creates New Event File When Plugin Raises A New Event
    [Tags]  SMOKE  MANAGEMENT_AGENT
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
    ${logPath} =  Set Variable  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log

    # make sure no previous event xml file exists.
    Remove Event Xml Files

    Setup Plugin Registry
    Start Management Agent

    Send Message To Management Agent Without Protobuf Serialisation  Bad Message 1

    ${managementLog} =  Get File  ${logPath}
    Should Contain X Times  ${managementLog}  ${errorMessage}  1  Management agent should contain : "${errorMessage}" only once

    Start Plugin

    Send Message To Management Agent Without Protobuf Serialisation  Bad Message 2

    ${eventContent} =     Evaluate    str('<a>event1</a>')
    Send Plugin Event   ${eventContent}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Event File     ${eventContent}

    ${managementLog} =  Get File  ${logPath}
    Should Contain X Times  ${managementLog}  ${errorMessage}  2  Management agent should contain : "${errorMessage}" only twice


*** Keywords ***

Plugin Event Test Teardown
    General Test Teardown
    Stop Plugin
    Stop Management Agent
    Terminate All Processes  kill=True