*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LiveResponseUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/PushServerUtils.py
Library     ${LIBS_DIRECTORY}/WebsocketWrapper.py

Library     String
Library     OperatingSystem

Resource    ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsPushClientResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot


Test Setup  Liveresponse Test Setup
Test Teardown  Liveresponse Test Teardown

Suite Setup   Liveresponse Suite Setup
Suite Teardown   Liveresponse Suite Teardown

Default Tags   LIVERESPONSE_PLUGIN  MANAGEMENT_AGENT  MCSROUTER  SMOKE

*** Variables ***
${Thumbprint}               F00ADD36235CD85DF8CA4A98DE979BDD3C48537EBEB4ADB1B31F9292105A5CA0
${websocket_server_url}     wss://localhost

*** Test Cases ***
Verify Liveresponse Works End To End LiveResponse Session Command Via Push
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${correlation_id} =  Get Correlation Id
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id}

    Send Message With Newline   ls ${SOPHOS_INSTALL}/plugins/liveresponse/bin/   ${correlation_id}
    wait_for_match_message   sophos-live-terminal   ${correlation_id}

    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id}

Verify Liveresponse Disconnects On Service Restart
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${correlation_id} =  Get Correlation Id
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id}

    Send Message With Newline   ls ${SOPHOS_INSTALL}/plugins/liveresponse/bin/   ${correlation_id}
    wait_for_match_message  sophos-live-terminal   ${correlation_id}

    ${count} =  Count Files In Directory  /opt/sophos-spl/plugins/liveresponse/var

    Should Be Equal As Integers  1   ${count}

    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id}

    Run Process   systemctl  restart  sophos-spl

    ${count} =  Count Files In Directory  /opt/sophos-spl/plugins/liveresponse/var

    Should Be Equal As Integers  0   ${count}


Multiple Liveresponse Sessions Work Concurrently
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${correlation_id1} =  Get Correlation Id
    ${correlation_id2} =  Get Correlation Id
    ${correlation_id3} =  Get Correlation Id
    ${correlation_id4} =  Get Correlation Id
    ${correlation_id5} =  Get Correlation Id
    ${correlation_id6} =  Get Correlation Id
    ${correlation_id7} =  Get Correlation Id
    ${correlation_id8} =  Get Correlation Id
    ${correlation_id9} =  Get Correlation Id
    ${correlation_id10} =  Get Correlation Id

    Check Liveresponse Command Successfully Starts A Session   ${correlation_id1}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id2}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id3}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id4}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id5}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id6}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id7}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id8}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id9}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id10}

    Send Message With Newline   ls ${SOPHOS_INSTALL}/plugins/liveresponse/bin/   ${correlation_id1}
    wait for match message   sophos-live-terminal   ${correlation_id1}

    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id1}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id2}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id3}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id4}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id5}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id6}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id7}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id8}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id9}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id10}

    ${files} =  List Files In Directory  /opt/sophos-spl/plugins/liveresponse/var
    Log  ${files}

    ${count} =  Count Files In Directory  /opt/sophos-spl/plugins/liveresponse/var
    Should Be Equal As Integers  ${count}  ${10}

    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id1}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id2}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id3}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id4}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id5}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id6}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id7}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id8}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id9}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id10}

Closing a LiveResponse Session Does Not Affect The Outcome Of Another Session
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${correlation_id1} =  Get Correlation Id
    ${correlation_id2} =  Get Correlation Id
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id1}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id1}

    Check Liveresponse Command Successfully Starts A Session   ${correlation_id2}
    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id2}

    Number Of Files In Dir Should Be  /opt/sophos-spl/plugins/liveresponse/var  2

    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id2}

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Number Of Files In Dir Should Be  /opt/sophos-spl/plugins/liveresponse/var  1

    Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id1}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id1}

Changing Environment Variables Does Not Affect Other LiveResponse Sessions
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct

    ${correlation_id1} =  Get Correlation Id
    ${correlation_id2} =  Get Correlation Id
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id1}
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id2}

    Set Environment Variable In Session  ${correlation_id1}  TEST_ENV  example_value_1
    Set Environment Variable In Session  ${correlation_id2}  TEST_ENV  example_value_2

    Send Message With Newline   echo $TEST_ENV   ${correlation_id1}
    wait for match message  example_value_1   ${correlation_id1}

    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id2}
    Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id1}

*** Keywords ***

Number Of Files In Dir Should Be
    [Arguments]  ${directoy}  ${expected_file_count}
    ${actual_file_count} =  Count Files In Directory  ${directoy}
    Should Be Equal As Integers  ${actual_file_count}  ${expected_file_count}

Check Liveresponse Command Successfully Starts A Session
    [Arguments]   ${correlationId}
    ${mark} =  Mark Managementagent Log
    ${liveResponse} =  Create Live Response Action  ${websocket_server_url}/${correlationId}  ${Thumbprint}  ${correlationId}
    Send Message To Push Server   ${liveResponse}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command from Push Server
    wait_for_log_contains_from_mark  ${mark}  Action LiveTerminal_${correlationId}_action_  timeout=${5}
    wait for match message  root@   ${correlationId}  timeout=${5}

Check Touch Creates Files Successfully From Liveresponse Session
    [Arguments]  ${path}
    Send Message With Newline   touch /tmp/test_${path}.txt   ${path}
    Wait Until Created    /tmp/test_${path}.txt  timeout=5 secs
    Remove File   /tmp/test_${path}.txt

Set Environment Variable In Session
    [Arguments]  ${correlationId}  ${envVariable}  ${value}
    Send Message With Newline   export ${envVariable}=${value}   ${correlationId}
    Send Message With Newline   echo $${envVariable}   ${correlationId}
    wait for match message  ${value}   ${correlationId}

Check Liveresponse Session Will Stop When Instructed by Central
    [Arguments]  ${session_path}
    Send Message With Newline   exit   ${session_path}
    Check Liveresponse Agent Executable is Not Running
