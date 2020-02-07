*** Settings ***
Library  OperatingSystem
Library    ${libs_directory}/PushServerUtils.py
Library    ${libs_directory}/MCSRouter.py

Resource  McsRouterResources.robot

Resource  ../GeneralTeardownResource.robot

Test Teardown  Test Teardown

Default Tags  FAKE_CLOUD

*** Test Case ***
MCSPushServer Can Be Used To Send Push Server Message
    Start MCS Push Server
    Start SSE Client
    Send Message To Push Server   Test Message
    ${receivedMessage}=  Next SSE Message
    Should be Equal  ${receivedMessage}  Test Message
    Check MCS Push Message Sent    Yet another message

# I intend to leave this test as a reminder that there is a possibility that
# disconnection from server will not be detected by the sse client that we
# have chosen. We will need to check with Central how to 'rule out' that this could be
# a problem.
# FIXME: LINUXDAR-1400 - Verify this is not a problem with the real push server from central
Client Should Detect Push Server Disconnection
    [Tags]  TESTFAILURE  FAKE_CLOUD
    Start MCS Push Server
    Start SSE Client
    Check MCS Push Message Sent    Single Message
    Check Server Stops Connection

Client Should Detect Push Server Goes Away
    Start MCS Push Server
    Start SSE Client
    Check MCS Push Message Sent    Single Message
    Server Close
    ${receivedMessage}=  Next SSE Message
    Should be Equal  ${receivedMessage}  abort

Client Should Detect Push Server Not Pinging Any More
    Start MCS Push Server
    Configure Push Server To Ping Interval  300
    Start SSE Client  timeout=1
    Run Keyword and Expect Error   STARTS: timeout:  Next SSE Message

MCS Push Server Can Handle Authorization
    Start MCS Push Server
    Configure Push Server To Require Auth   Basic XCnos9s
    Check Client Not Authorized     Basic With Different Creds
    Start SSE Client    authorization=Basic XCnos9s
    Check MCS Push Message Sent    Single Message


MCSPushServer Can Control The Ping Interval
    Start MCS Push Server
    Configure Push Server To Ping Interval  3
    Start SSE Client
    Check Ping Time Interval  3

MCS Fake Server Redirects To MCS Push Server
    [Teardown]  Teardown with MCS Fake Server
    Start Local Cloud Server
    Start MCS Push Server
    Verify Cloud Server Redirect
    Start SSE Client    via_cloud_server=True
    Check MCS Push Message Sent    Single Message
    # 2 times, one for the 'verify cloud server redirect' and the other via_cloud_server
    Check Cloud Server Log Contains   Push redirect requested    2





*** Keywords ***

Log Content of MCS Push Server Log
    ${LogContent}=  MCS Push Server Log
    Log  ${LogContent}

Teardown with MCS Fake Server
    MCSRouter Test Teardown
    Stop Local Cloud Server
    Cleanup Local Cloud Server Logs
    Test Teardown   runGeneral=False


Test Teardown
    [Arguments]  ${runGeneral}=True
    Shutdown MCS Push Server
    Run Keyword If Test Failed    Log Content of MCS Push Server Log
    Run Keyword If  ${runGeneral}  General Test Teardown