*** Settings ***
Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/PushServerUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Test Teardown  Push Server Test Teardown

Force Tags  FAKE_CLOUD  TAP_PARALLEL4

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
    # TODO: LINUXDAR-8285: Fix use of openssl on TAP templates so that this test can run
    [Tags]    TESTFAILURE
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
    Run Keyword and Expect Error   GLOB: *The read operation timed out*  Next SSE Message

MCSPushServer Can Control The Ping Interval
    Start MCS Push Server
    Configure Push Server To Ping Interval  3
    Start SSE Client
    Check Ping Time Interval  3

MCS Fake Server Redirects To MCS Push Server
    [Teardown]  Push Server Teardown with MCS Fake Server
    Start Local Cloud Server
    Start MCS Push Server
    Verify Cloud Server Redirect
    Start SSE Client    via_cloud_server=True
    Check MCS Push Message Sent    Single Message
    # 2 times, one for the 'verify cloud server redirect' and the other via_cloud_server
    Check Cloud Server Log Contains   Push redirect requested    2