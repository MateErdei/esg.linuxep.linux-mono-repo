*** Settings ***
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/OnFail.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup     RA Suite Setup
Suite Teardown  RA Suite Teardown

Test Setup  RA Test Setup
Test Teardown  RA Upload Proxy Test Teardown
Force Tags  LOAD5
Default Tags   RESPONSE_ACTIONS_PLUGIN

*** Test Cases ***
RA Plugin uploads a file successfully with proxy
    Start Simple Proxy Server    3333
    Set Environment Variable  https_proxy   http://localhost:3333
    Register With Local Cloud Server
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Upload_File_From_Fake_Cloud   /tmp/file  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Upload for /opt/sophos-spl/plugins/responseactions/tmp/file.zip succeeded   15
    wait_for_log_contains_from_mark  ${action_mark}  Uploading via proxy: localhost:3333
    Check Log Does Not Contain     Connection with proxy failed going direct   ${ACTIONS_RUNNER_LOG_PATH}    actionrunner

RA Plugin uploads a file successfully with proxy with password
    Require Proxy With Basic Authentication  4444
    Set Environment Variable  https_proxy  http://user:password@localhost:4444
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Register With Local Cloud Server
    Send_Upload_File_From_Fake_Cloud   /tmp/file  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Upload for /opt/sophos-spl/plugins/responseactions/tmp/file.zip succeeded   15
    wait_for_log_contains_from_mark  ${action_mark}  Uploading via proxy: localhost:4444
    Check Log Does Not Contain     Connection with proxy failed going direct   ${ACTIONS_RUNNER_LOG_PATH}    actionrunner

RA Plugin uploads a file successfully with message relay
    Start Simple Proxy Server    3000

    Register With Local Cloud Server
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    ${mcs_mark} =  mark_log_size  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>

    wait_for_log_contains_from_mark  ${mcs_mark}  Successfully connected to localhost:4443 via localhost:3000   25
    Create File  /tmp/file  tempfilecontent
    Register Cleanup  Remove File  /tmp/file
    Send_Upload_File_From_Fake_Cloud   /tmp/file  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   60
    wait_for_log_contains_from_mark  ${action_mark}  Upload for /opt/sophos-spl/plugins/responseactions/tmp/file.zip succeeded   15
    wait_for_log_contains_from_mark  ${action_mark}  Uploading via proxy: localhost:3000
    Check Log Does Not Contain     Connection with proxy failed going direct   ${ACTIONS_RUNNER_LOG_PATH}    actionrunner

RA Plugin uploads a folder successfully with proxy
    Start Simple Proxy Server    3333
    Set Environment Variable  https_proxy   http://localhost:3333
    Register With Local Cloud Server
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create Directory  /tmp/compressionTest
    Create File  /tmp/compressionTest/file.txt  tempfilecontent
    Register Cleanup  Remove Directory  /tmp/compressionTest    recursive=${True}
    Send_Upload_Folder_From_Fake_Cloud   /tmp/compressionTest  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Upload for ${SOPHOS_INSTALL}/plugins/responseactions/tmp/compressionTest.zip succeeded   15
    wait_for_log_contains_from_mark  ${action_mark}  Uploading via proxy: localhost:3333
    Check Log Does Not Contain     Connection with proxy failed going direct   ${ACTIONS_RUNNER_LOG_PATH}    actionrunner

RA Plugin uploads a folder successfully with proxy with password
    Require Proxy With Basic Authentication  4444
    Set Environment Variable  https_proxy  http://user:password@localhost:4444
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    Create Directory  /tmp/compressionTest
    Create File  /tmp/compressionTest/file.txt  tempfilecontent
    Register Cleanup  Remove Directory  /tmp/compressionTest    recursive=${True}
    Register With Local Cloud Server
    Send_Upload_Folder_From_Fake_Cloud   /tmp/compressionTest  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   25
    wait_for_log_contains_from_mark  ${action_mark}  Upload for ${SOPHOS_INSTALL}/plugins/responseactions/tmp/compressionTest.zip succeeded   15
    wait_for_log_contains_from_mark  ${action_mark}  Uploading via proxy: localhost:4444
    Check Log Does Not Contain     Connection with proxy failed going direct   ${ACTIONS_RUNNER_LOG_PATH}    actionrunner

RA Plugin uploads a folder successfully with message relay
    Start Simple Proxy Server    3000

    Register With Local Cloud Server
    ${response_mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${action_mark} =  mark_log_size  ${ACTIONS_RUNNER_LOG_PATH}
    ${mcs_mark} =  mark_log_size  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>

    wait_for_log_contains_from_mark  ${mcs_mark}  Successfully connected to localhost:4443 via localhost:3000   25
    Create Directory  /tmp/compressionTest
    Create File  /tmp/compressionTest/file.txt  tempfilecontent
    Register Cleanup  Remove Directory  /tmp/compressionTest    recursive=${True}
    Send_Upload_Folder_From_Fake_Cloud   /tmp/compressionTest  ${TRUE}  corrid  password
    wait_for_log_contains_from_mark  ${response_mark}  Action corrid has succeeded   60
    wait_for_log_contains_from_mark  ${action_mark}  Upload for ${SOPHOS_INSTALL}/plugins/responseactions/tmp/compressionTest.zip succeeded   15
    wait_for_log_contains_from_mark  ${action_mark}  Uploading via proxy: localhost:3000
    Check Log Does Not Contain     Connection with proxy failed going direct   ${ACTIONS_RUNNER_LOG_PATH}    actionrunner

*** Keywords ***
RA Upload Proxy Test Teardown
    RA Test Teardown
    Stop Proxy Servers
    Stop Proxy If Running
    Cleanup Proxy Logs
    Cleanup Register Central Log
    Deregister From Central
    Remove Environment Variable  https_proxy

Require Proxy With Basic Authentication
    [Arguments]    ${proxy_port}
    Start Proxy Server With Basic Auth  ${proxy_port}  user  password