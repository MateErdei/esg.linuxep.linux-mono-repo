*** Settings ***
#Library    Process
#Library    OperatingSystem
Library    ../libs/FullInstallerUtils.py
Library    ../libs/UpdateServer.py

Resource  ../GeneralTeardownResource.robot

Suite Setup      Local Suite Setup
Suite Teardown   Local Suite Teardown

*** Variables ***
${server_handle}=  None

*** Keywords ***
Local Suite Setup
    # Install base so that the test binary can use the libs shipped by the product
    Run Full Installer
    ${server_handle}=  Start Process   ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpTestServer.py
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Can Curl Url    http://localhost:7780

Local Suite Teardown
    Terminate Process 	${server_handle}

*** Test Case ***
Http Library Tests
    ${result} =  Run Process   ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/HttpRequesterLiveNetworkTests
    Should Be Equal As Integers  ${result.rc}  0  msg="HttpRequesterLiveNetworkTests tests failed. \n stdout: ${result.stdout} stderr: ${result.stderr}."