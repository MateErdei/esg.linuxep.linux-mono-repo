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
${PARENT_WORKING_DIR}   /tmp/parent

${FileNameRequest1}   ${PARENT_WORKING_DIR}/test.json
${ExpectedResponse1}  test.jsonresp.json
${ExpectedResponse1Jail}  ${PARENT_WORKING_DIR}/test.jsonresp.json

${CERT_PATH}   /tmp/cert.pem
${JAIL_PINNED_CERT_PATH}   /base/mcs/certs/cert.pem
${MCS_CERTS_DIR}   ${SOPHOS_INSTALL}/base/mcs/certs/
${PORT}        10560
${HTTPS_LOG_FILE}     https_server.log
${HTTPS_LOG_FILE_PATH}     /tmp/${HTTPS_LOG_FILE}
${JAIL_PATH}   /tmp/jail


*** Test Cases ***

Test RunHttpRequest without Jail can perform a GET request with pinned Certificate
    Create Http Json Request  ${FileNameRequest1}  requestType=GET  server=localhost  port=${PORT}   certPath=${CERT_PATH}
    ${output}=  Run Shell Process  ${RunHttpRequestExecutable} -i ${FileNameRequest1}   "Failed to run http request"  30  expectedExitCode=0
    Set Test Variable  ${RunHttpRequestLog}   ${output}
    ${content}=  Extract BodyContent Of Json Response  ${ExpectedResponse1}  httpCode=200
    Should Contain   ${content}   Response From HttpsServer


Test RunHttpRequest without Jail can perform a PUT request with pinned Certificate
    ${headers} =   Create List   info1: header1  info2: header2
    Create Http Json Request  ${FileNameRequest1}  requestType=PUT  server=localhost  port=${PORT}   certPath=${CERT_PATH}   headers=${headers}  bodyContent=test RunHttpRequest
    ${output} =  Run Shell Process  ${RunHttpRequestExecutable} -i ${FileNameRequest1}   "Failed to run http request"  30  expectedExitCode=0
    Set Test Variable  ${RunHttpRequestLog}   ${output}
    ${log} =  Get File  ${HTTPS_LOG_FILE_PATH}
    Should Contain   ${log}   header2
    Should Contain   ${log}   test RunHttpRequest
    ${content}=  Extract BodyContent Of Json Response  ${ExpectedResponse1}  httpCode=200
    Should Be Empty   ${content}


Test RunHttpRequest with Jail can perform a GET request with pinned Certificate
    Copy File And Set Permissions   ${CERT_PATH}  ${MCS_CERTS_DIR}
    Create Http Json Request  ${FileNameRequest1}  requestType=GET  server=localhost  port=${PORT}   certPath=${JAIL_PINNED_CERT_PATH}
    Run Jailed Https Request
    ${content}=  Extract BodyContent Of Json Response  ${ExpectedResponse1Jail}  httpCode=200
    Should Contain   ${content}   Response From HttpsServer


Test RunHttpRequest with Jail can perform a PUT request with pinned Certificate
    ${headers} =   Create List   info1: header1  info2: header2
    Create Http Json Request  ${FileNameRequest1}  requestType=PUT  server=localhost  port=${PORT}   certPath=${JAIL_PINNED_CERT_PATH}   headers=${headers}  bodyContent=test RunHttpRequest
    Run Jailed Https Request
    ${log} =  Get File  ${HTTPS_LOG_FILE_PATH}
    Should Contain   ${log}   header2
    Should Contain   ${log}   test RunHttpRequest
    ${content}=  Extract BodyContent Of Json Response  ${ExpectedResponse1Jail}  httpCode=200
    Should Be Empty   ${content}

*** Keywords ***

Suite Setup
    Require Fresh Install
    Stop System Watchdog

Suite Teardown
    Cleanup mounts


Test Setup
    Should Exist  ${RunHttpRequestExecutable}    
    HttpsServer.Start Https Server  ${CERT_PATH}  ${PORT}  tlsv1_2
    Copy File And Set Permissions   ${CERT_PATH}  ${MCS_CERTS_DIR}
    Create Directory And Setup Permissions  ${PARENT_WORKING_DIR}  sophos-spl-local   sophos-spl-group

Test Teardown
    Run Keyword If Test Failed  LogUtils.Dump Log  ${FileNameRequest1}
    Run Keyword If Test Failed  LogUtils.Dump Log    ${ExpectedResponse1}
    Run Keyword If Test Failed  LogUtils.Dump Log    ${HTTPS_LOG_FILE_PATH}
    Run Keyword If Test Failed  Log    ${RunHttpRequestLog}
    Remove File   ${FileNameRequest1}
    Remove File   ${ExpectedResponse1}
    Remove File   ${HTTPS_LOG_FILE_PATH}
    Stop Https Server
    Verify All Mounts Have Been Removed
    General Test Teardown



Cleanup mounts
    Run Process  umount  ${JAIL_PATH}/etc/hosts
    Run Process  umount  ${JAIL_PATH}/etc/resolv.conf
    Run Process  umount  ${JAIL_PATH}/lib
    Run Process  umount  ${JAIL_PATH}/usr/lib
    Run Process  umount  ${JAIL_PATH}/etc/ssl/certs
    Run Process  umount  ${JAIL_PATH}/etc/pki/tls/certs
    Run Process  umount  ${JAIL_PATH}/base/mcs/certs/

Chmod
    [Arguments]  ${settings}  ${path}
    ${r} =  Run Process  chmod  ${settings}  ${path}
    Should Be Equal As Strings  ${r.rc}  0

Chown
    [Arguments]  ${settings}  ${path}
    ${r} =  Run Process  chown  ${settings}  ${path}
    Should Be Equal As Strings  ${r.rc}  0

Copy File And Set Permissions
    [Arguments]  ${srcfile}  ${dstdir}
    ${directory} =  Get Dirname Of Path  ${srcfile}
    ${basename} =  Get Basename Of Path  ${srcfile}

    Copy File  ${srcfile}    ${dstdir}/${basename}
    ${r} =  Run Process  chown  root:sophos-spl-group  ${dstdir}/${basename}
    Should Be Equal As Strings  ${r.rc}  0

Create Directory And Setup Permissions
    [Arguments]   ${directoryPath}  ${user}  ${group}
    Create Directory   ${directoryPath}
    ${r} =  Run Process  chown  ${user}:${group}  ${directoryPath}

Run Jailed Https Request
    ${output} =  Run Shell Process  ${RunHttpRequestExecutable} -i ${FileNameRequest1} --child-user sophos-spl-network --child-group sophos-spl-group --parent-user sophos-spl-local --parent-group sophos-spl-group --jail-root ${JAIL_PATH} --parent-root /tmp/parent  "Failed to run http request"  30  expectedExitCode=0
    Set Test Variable  ${RunHttpRequestLog}   ${output}
    ${output} =  Run Shell Process  ${RunHttpRequestExecutable} --jail-root ${JAIL_PATH}  "Failed to unmount path"  5  expectedExitCode=0
    Log   ${output}


Verify All Mounts Have Been Removed
    Check Not A MountPoint  ${JAIL_PATH}/etc/resolv.conf
    Check Not A MountPoint  ${JAIL_PATH}/etc/hosts
    Check Not A MountPoint  ${JAIL_PATH}/usr/lib
    Check Not A MountPoint  {JAIL_PATH}/lib
    Check Not A MountPoint  ${JAIL_PATH}/etc/ssl/certs
    Check Not A MountPoint  ${JAIL_PATH}/etc/pki/tls/certs
    Check Not A MountPoint  ${JAIL_PATH}/base/mcs/certs

Check Not A MountPoint
    [Arguments]  ${mount}
    ${res} =  Run Process  findmnt  -M  ${mount}
    Should Not Be Equal As Integers   ${res.rc}  0