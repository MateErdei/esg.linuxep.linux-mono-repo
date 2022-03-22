*** Settings ***
Library    ../libs/FullInstallerUtils.py
Library    ../libs/UpdateServer.py

Resource  ../GeneralTeardownResource.robot

Suite Setup      Local Suite Setup
Suite Teardown   Local Suite Teardown

Default Tags  TAP_TESTS

*** Variables ***
${server_handle}=  None

*** Keywords ***
Local Suite Setup
    # Install base so that the test binary can use the libs shipped by the product
    Run Full Installer

    # Start HTTP, HTTPS, proxy and authenticated proxy servers:
    Should Exist  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpTestServer.py
    ${server_handle}=  Start Process   ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpTestServer.py
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Can Curl Url    http://localhost:7780

Local Suite Teardown
    Terminate Process 	${server_handle}
    Log File  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/http_test_server.log

*** Test Case ***
Http Library Tests
    ${result} =  Run Process   ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpRequesterLiveNetworkTests
    Log ${result.stdout}
    Log ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0  msg="HttpRequesterLiveNetworkTests tests failed. \n stdout: ${result.stdout} stderr: ${result.stderr}."