*** Settings ***
Library    ../libs/FullInstallerUtils.py
Library    ../libs/UpdateServer.py
Library    ../libs/ProcessUtils.py

Resource  ../GeneralTeardownResource.robot

Suite Setup      Local Suite Setup
Suite Teardown   Local Suite Teardown

Default Tags  TAP_TESTS

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
    ${server_pid} =  run_process_in_background  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpTestServer.py
    Set Suite Variable  ${server_pid}   ${server_pid}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Can Curl Url    http://localhost:7780
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/localhost-selfsigned.crt
    ${test_rc} =  run_and_wait_for_process  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpRequesterLiveNetworkTests
    Should Be Equal As Integers  ${test_rc}  0  msg="HttpRequesterLiveNetworkTests tests failed"
