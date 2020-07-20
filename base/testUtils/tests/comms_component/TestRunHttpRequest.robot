*** Settings ***


Resource  ../watchdog/WatchdogResources.robot
Resource  ../upgrade_product/UpgradeResources.robot
Resource  CommsComponentResources.robot
Resource  ../installer/InstallerResources.robot

Library   ${LIBS_DIRECTORY}/CommsComponentUtils.py
Library   ${LIBS_DIRECTORY}/LogUtils.py
Library   ${LIBS_DIRECTORY}/HttpsServer.py

Default Tags  COMMS  TAP_TESTS

Suite Setup  Suite Setup
Suite Teardown  Suite Teardown
Test Setup  Test Setup
Test Teardown  Test Teardown

*** Variables ***

${RunHttpRequestExecutable}  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/RunHttpRequest
${FileNameRequest1}   test.json
${ExpectedResponse1}  test.jsonresp.json
${CERT_PATH}   /tmp/cert.pem
${PORT}        10560
${HTTPS_LOG_FILE}     https_server.log
${HTTPS_LOG_FILE_PATH}     /tmp/${HTTPS_LOG_FILE}

*** Test Cases ***

Test RunHttpRequest without Jail can perform a GET request with pinned Certificate
    Create Http Json Request  test.json  requestType=GET  server=localhost  port=${PORT}   certPath=${CERT_PATH}
    ${output}=  Run Shell Process  ${RunHttpRequestExecutable} -i ${FileNameRequest1}   "Failed to run http request"  30  expectedExitCode=0
    Set Test Variable  ${RunHttpRequestLog}   ${output}
    ${content}=  Extract BodyContent Of Json Response  ${ExpectedResponse1}  httpCode=200
    Should Contain   ${content}   Response From HttpsServer


Test RunHttpRequest without Jail can perform a PUT request with pinned Certificate
    ${headers} =   Create List   info1: header1  info2: header2
    Create Http Json Request  test.json  requestType=PUT  server=localhost  port=${PORT}   certPath=${CERT_PATH}   headers=${headers}  bodyContent=test RunHttpRequest
    ${output} =  Run Shell Process  ${RunHttpRequestExecutable} -i ${FileNameRequest1}   "Failed to run http request"  30  expectedExitCode=0
    Set Test Variable  ${RunHttpRequestLog}   ${output}
    ${log} =  Get File  ${HTTPS_LOG_FILE_PATH}
    Should Contain   ${log}   header2
    Should Contain   ${log}   test RunHttpRequest
    ${content}=  Extract BodyContent Of Json Response  ${ExpectedResponse1}  httpCode=200
    Should Be Empty   ${content}


*** Keywords ***

Suite Setup
    Require Fresh Install
    Stop System Watchdog

Suite Teardown
    No Operation


Test Setup
    Should Exist  ${RunHttpRequestExecutable}    
    HttpsServer.Start Https Server  ${CERT_PATH}  ${PORT}  tlsv1_2    

Test Teardown
    Run Keyword If Test Failed  LogUtils.Dump Log  ${FileNameRequest1}
    Run Keyword If Test Failed  LogUtils.Dump Log    ${ExpectedResponse1}
    Run Keyword If Test Failed  LogUtils.Dump Log    ${HTTPS_LOG_FILE_PATH}
    Run Keyword If Test Failed  Log    ${RunHttpRequestLog}
    Remove File   ${FileNameRequest1}
    Remove File   ${ExpectedResponse1}
    Remove File   ${HTTPS_LOG_FILE_PATH}
    Stop Https Server    
    General Test Teardown