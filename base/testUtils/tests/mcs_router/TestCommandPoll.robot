*** Settings ***
Library    ${LIBS_DIRECTORY}/LogUtils.py
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


Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

*** Test Case ***
Command Poll Sent
    [Tags]  SMOKE  MCS  FAKE_CLOUD  MCS_ROUTER
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
    start_system_watchdog
    stop_local_cloud_server

    Start Update Scheduler

    Set Test Variable  ${sulConfigPath}  ${SOPHOS_INSTALL}/base/update/var/update_config.json
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Sleep   10s   #wait to settle down
    Simulate Update Now

    ${MCSRouterLog}=   Set Variable   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains    waiting up to 20.000000s after 1 failures    ${MCSRouterLog}    mcsrouter

    Wait Until Keyword Succeeds
    ...  21 secs
    ...  1 secs
    ...  Check Log Contains   waiting up to 40.000000s after 2 failures    ${MCSRouterLog}    mcsrouter

    Wait Until Keyword Succeeds
    ...  41 secs
    ...  1 secs
    ...  Check Log Contains   waiting up to 80.000000s after 3 failures    ${MCSRouterLog}    mcsrouter


    Check Log Contains String N Times   ${MCSRouterLog}   MCS Router Log   waiting up to 20.000000s after 1 failures   1
    Check Log Contains String N Times   ${MCSRouterLog}   MCS Router Log   waiting up to 40.000000s after 2 failures   1
    Check Log Contains String N Times   ${MCSRouterLog}   MCS Router Log   waiting up to 80.000000s after 3 failures   1

    Check MCS Router Log Contains In Order
    ...   waiting up to 20.000000s after 1 failures
    ...   waiting up to 40.000000s after 2 failures
    ...   waiting up to 80.000000s after 3 failures