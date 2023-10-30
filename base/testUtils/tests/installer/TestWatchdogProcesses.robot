*** Settings ***
Documentation    Tests around Watchdog and process spawning

Resource  ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource  ${COMMON_TEST_ROBOT}/InstallerResources.robot

Force Tags  TAP_PARALLEL5

*** Test Cases ***

Test File Descriptors Are Closed When WD Spawns Process
    [Tags]    WATCHDOG
    Require Fresh Install

    ${rc}   ${wd_pid} =    Run And Return Rc And Output    pgrep sophos_watchdog
    Should Be Equal As Integers    ${rc}    0

    ${rc}   ${updater_pid} =    Run And Return Rc And Output    pgrep UpdateScheduler
    Should Be Equal As Integers    ${rc}    0

    ${result} =    Run Process  ls  -l  /proc/${wd_pid}/fd
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    ${wd_fd} =  Set Variable   ${result.stdout}

    ${result} =    Run Process  ls  -l  /proc/${updater_pid}/fd
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    ${updater_fd} =  Set Variable   ${result.stdout}

    Should Contain  ${wd_fd}  /opt/sophos-spl/logs/base/watchdog.log
    Should Not Contain  ${updater_fd}  /opt/sophos-spl/logs/base/watchdog.log

