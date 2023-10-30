*** Settings ***
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/ProcessUtils.py
Library     ${COMMON_TEST_LIBS}/UpdateServer.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot

Suite Setup      Local Suite Setup
Suite Teardown   Local Suite Teardown

Force Tags  TAP_PARALLEL2

*** Keywords ***
Local Suite Setup
    # Install base so that the test binary can use the libs shipped by the product
    Run Full Installer
    Should Exist  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpTestServer.py

Local Suite Teardown
    kill_process  ${server_pid}
    Log File  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/http_test_server.log

*** Test Case ***
Http Library Tests
    # Exclude on SLES until LINUXDAR-7306 is fixed
    [Tags]  EXCLUDE_SLES12
    ${server_pid} =  run_process_in_background  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpTestServer.py
    Set Suite Variable  ${server_pid}   ${server_pid}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Can Curl Url    http://localhost:7780

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Can Curl Url    http://localhost:7780  proxy=http://localhost:7750

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/localhost-selfsigned.crt

    ${rc}  ${stdout}  ${stderr}    run_and_wait_for_process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpRequesterLiveNetworkTests
    log  ${stdout}
    log  ${stderr}
    Should Be Equal As Integers  ${rc}  0  msg="HttpRequesterLiveNetworkTests tests failed"
