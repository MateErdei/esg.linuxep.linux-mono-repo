*** Settings ***
Documentation    Suite description

Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/LiveResponseUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/PushServerUtils.py
Library     ${COMMON_TEST_LIBS}/WebsocketWrapper.py

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

# TODO: SLES does not have "root@" in the command prompt so difficult to confirm live terminal session is working
Force Tags   LIVERESPONSE_PLUGIN  MANAGEMENT_AGENT  MCSROUTER  SMOKE  EXCLUDE_SLES12  EXCLUDE_SLES15

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

    @{correlation_ids} =  Get Correlation Ids    ${10}

    FOR    ${correlation_id}    IN    @{correlation_ids}
        Check Liveresponse Command Successfully Starts A Session   ${correlation_id}
    END

    FOR    ${correlation_id}    IN    @{correlation_ids}
        Send Message With Newline   ls ${SOPHOS_INSTALL}/plugins/liveresponse/bin/   ${correlation_id}
        wait for match message   sophos-live-terminal   ${correlation_id}
    END

    FOR    ${correlation_id}    IN    @{correlation_ids}
        Check Touch Creates Files Successfully From Liveresponse Session   ${correlation_id}
    END

    ${files} =  List Files In Directory  /opt/sophos-spl/plugins/liveresponse/var
    Log  ${files}

    FOR    ${correlation_id}    IN    @{correlation_ids}
        Check Liveresponse Session Will Stop When Instructed by Central   ${correlation_id}
    END


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

Verify Liveresponse Creates Files With Correct Permissions
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Direct
    ${correlation_id} =  Get Correlation Id
    Check Liveresponse Command Successfully Starts A Session   ${correlation_id}
    Send Message With Newline   ls ${SOPHOS_INSTALL}/plugins/liveresponse/bin/   ${correlation_id}
    wait_for_match_message   sophos-live-terminal   ${correlation_id}

    # Create a file using live response and check the permissions are correct
    ${path}=  Set Variable  /tmp/test_${correlation_id}.txt
    Send Message With Newline   touch ${path}   ${correlation_id}
    Wait Until Created    /tmp/test_${correlation_id}.txt  timeout=5 secs
    Register Cleanup   Remove File    ${path}

    Send Message With Newline   umask > /tmp/umask.txt   ${correlation_id}
    Wait Until Created    /tmp/umask.txt  timeout=5 secs
    Register Cleanup   Remove File    /tmp/umask.txt

    ${rc}   ${output} =    Run And Return Rc And Output    cat /tmp/umask.txt
    Log  return code is ${rc}
    Log  output is ${output}

    ${rc}   ${output} =    Run And Return Rc And Output    grep -r grep -3 -r umask /etc
    Log  return code is ${rc}
    Log  output is ${output}

    ${rc}   ${output} =    Run And Return Rc And Output    cat /etc/profile
    Log  return code is ${rc}
    Log  output is ${output}

    ${rc}   ${output} =    Run And Return Rc And Output    cat ~/.profile
    Log  return code is ${rc}
    Log  output is ${output}

    ${rc}   ${output} =    Run And Return Rc And Output  ls -l ${path}
    Log  return code is ${rc}
    Log  output is ${output}
    Should Contain    ${output}    -rw-------
    Should Contain    ${output}    root sophos-spl-group
    Should Be Equal As Integers  ${rc}  ${0}


*** Keywords ***

Number Of Files In Dir Should Be
    [Arguments]  ${directoy}  ${expected_file_count}
    ${actual_file_count} =  Count Files In Directory  ${directoy}
    Should Be Equal As Integers  ${actual_file_count}  ${expected_file_count}

Check Liveresponse Command Successfully Starts A Session
    [Arguments]   ${correlationId}
    ${ma_mark} =  Mark Managementagent Log
    ${mr_mark} =  Get Mark For Mcsrouter Log
    ${liveResponse} =  Create Live Response Action  ${websocket_server_url}/${correlationId}  ${Thumbprint}  ${correlationId}
    Send Message To Push Server   ${liveResponse}
    Wait For Log Contains From Mark    ${mr_mark}    Received command from Push Server
    Wait For Log Contains From Mark    ${ma_mark}    Action LiveTerminal_${correlationId}_action_  timeout=${5}
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
