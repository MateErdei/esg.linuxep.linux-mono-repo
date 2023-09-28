*** Settings ***
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource  McsRouterResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server
Suite Teardown    Run Keywords
...               Stop Local Cloud Server  AND
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_PARALLEL5

*** Test Case ***
Command Poll Sent
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Cloud Server Log For Command Poll

# Note that this test can fail on purely DNS based setups (socket.getfqdn).
# This can be fixed by adding own hostname to hostfile on test machine
Default Command Poll Interval
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Cloud Server Log For Command Poll    2

    # Check for default of 5 seconds with tolerance of 1 seconds
    Compare Default Command Poll Interval With Log    5    1

Test Connection Retry Backoff Period Is Set Correctly
    # When a change of update status occurs i.e. success to failed, ensure that the calculated back off period is correct.
    # Note that the back-off period is used as a timeout not a wait.
    # If the select.select python method is triggered the backoff/timeout will not be hit.
    # Also the backoff / timeout period set will be a random number bewteen the last backoff period and the new backoff period.

    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Default Policies Exist
    Send Policy File  mcs  ${SUPPORT_FILES}/CentralXml/Cloud_MCS_policy_15sPollingDelay.xml
    ${MCSRouterLog}=   Set Variable   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...   Check Log Contains String N Times   ${MCSRouterLog}   MCS Router Log  Distribute new policy: MCS-25_policy.xml  2
    Stop Local Cloud Server

    # check that backoff no longer breaks due to status being generated by update scheduler
    Send Status File  ALC_status.xml

    ${MCSRouterLog}=   Set Variable   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains    waiting up to 15.000000s after 1 failures    ${MCSRouterLog}    mcsrouter

    Wait Until Keyword Succeeds
    ...  21 secs
    ...  1 secs
    ...  Check Log Contains   waiting up to 30.000000s after 2 failures    ${MCSRouterLog}    mcsrouter

    Wait Until Keyword Succeeds
    ...  41 secs
    ...  1 secs
    ...  Check Log Contains   waiting up to 60.000000s after 3 failures    ${MCSRouterLog}    mcsrouter


    Check Log Contains String N Times   ${MCSRouterLog}   MCS Router Log   waiting up to 15.000000s after 1 failures   1
    Check Log Contains String N Times   ${MCSRouterLog}   MCS Router Log   waiting up to 30.000000s after 2 failures   1
    Check Log Contains String N Times   ${MCSRouterLog}   MCS Router Log   waiting up to 60.000000s after 3 failures   1

    Check MCS Router Log Contains In Order
    ...   waiting up to 15.000000s after 1 failures
    ...   waiting up to 30.000000s after 2 failures
    ...   waiting up to 60.000000s after 3 failures

    ${timestamp1} =  get_time_difference_between_two_log_lines_where_log_lines_are_in_order   waiting up to 15.000000s after 1 failures   Trying connection directly to localhost:4443     ${MCSRouterLog}
    IF  ${timestamp1} < 10 or 15.5 < ${timestamp1}
      Fail
    END

    ${timestamp2} =  get_time_difference_between_two_log_lines_where_log_lines_are_in_order   waiting up to 30.000000s after 2 failures   Trying connection directly to localhost:4443     ${MCSRouterLog}
    IF  ${timestamp2} < 15 or 30.5 < ${timestamp2}
      Fail
    END