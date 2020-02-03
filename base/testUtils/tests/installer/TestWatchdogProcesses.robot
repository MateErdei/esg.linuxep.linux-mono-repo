*** Settings ***
Documentation    Tests around Watchdog and process spawning

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

*** Test Cases ***

Test File Descriptors Are Closed When WD Spawns Process
    [Tags]    WATCHDOG
    Require Fresh Install

    ${result} =    Run Process  pidof  sophos_watchdog
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    ${wd_pid} =  Set Variable   ${result.stdout}

    ${result} =    Run Process  pidof  UpdateScheduler
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    ${updater_pid} =  Set Variable   ${result.stdout}

    ${result} =    Run Process  ls  -l  /proc/${wd_pid}/fd
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    ${wd_fd} =  Set Variable   ${result.stdout}

    ${result} =    Run Process  ls  -l  /proc/${updater_pid}/fd
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    ${updater_fd} =  Set Variable   ${result.stdout}

    Should Contain  ${wd_fd}  /opt/sophos-spl/logs/base/watchdog.log
    Should Not Contain  ${updater_fd}  /opt/sophos-spl/logs/base/watchdog.log

