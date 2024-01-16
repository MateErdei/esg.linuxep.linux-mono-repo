*** Settings ***
Documentation  Test the Telemetry Scheduler

Library     ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py
Library     ${COMMON_TEST_LIBS}/ProcessUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Suite Setup      Setup TelemetryScheduling Tests
Suite Teardown   Cleanup TelemetryScheduling Tests

Test Setup       Telemetry Scheduler Plugin Test Setup
Test Teardown    Telemetry Scheduler Plugin Test Teardown

Force Tags  TELEMETRY SCHEDULER    TAP_PARALLEL5

*** Keywords ***
### Suite Setup
Setup TelemetryScheduling Tests
    Require Fresh Install
    Copy File  ${TELEMETRY_CONFIG_FILE_SOURCE}  ${TELEMETRY_CONFIG_FILE}
    Override LogConf File as Global Level  DEBUG

### Suite Cleanup
Cleanup TelemetryScheduling Tests
    Uninstall SSPL

Telemetry Scheduler Plugin Test Setup
    Simulate Send Policy    ALC_policy_direct.xml
    Run Process  mv  ${TELEMETRY_EXECUTABLE}  ${TELEMETRY_EXECUTABLE}.orig
    Create Fake Telemetry Executable
    Set Interval In Configuration File  ${TEST_INTERVAL}
    ${ts_log}=    mark_log_size    ${TELEMETRY_SCHEDULER_LOG}
    Remove File  ${STATUS_FILE}
    Create File  ${STATUS_FILE}
    Restart Telemetry Scheduler
    wait_for_log_contains_from_mark    ${ts_log}    Telemetry reporting is scheduled to run at

Telemetry Scheduler Plugin Test Setup with error telemetry executable
    Simulate Send Policy    ALC_policy_direct.xml
    Run Process  mv  ${TELEMETRY_EXECUTABLE}  ${TELEMETRY_EXECUTABLE}.orig
    Create Fake Telemetry Executable that exits with error
    Set Interval In Configuration File  ${TEST_INTERVAL}
    ${ts_log}=    mark_log_size    ${TELEMETRY_SCHEDULER_LOG}
    Remove File  ${STATUS_FILE}
    Create File  ${STATUS_FILE}
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${STATUS_FILE}    shell=True
    Restart Telemetry Scheduler
    wait_for_log_contains_from_mark    ${ts_log}    Telemetry reporting is scheduled to run at

Telemetry Scheduler Plugin Test Setup Without Sending Policy
    Run Process  mv  ${TELEMETRY_EXECUTABLE}  ${TELEMETRY_EXECUTABLE}.orig
    Create Fake Telemetry Executable
    Set Interval In Configuration File  ${TEST_INTERVAL}
    ${ts_log}=    mark_log_size    ${TELEMETRY_SCHEDULER_LOG}
    Remove File  ${STATUS_FILE}
    Create File  ${STATUS_FILE}
    Restart Telemetry Scheduler
    wait_for_log_contains_from_mark    ${ts_log}    Telemetry reporting is scheduled to run at

Telemetry Scheduler Plugin Test Teardown
    General Test Teardown
    Run Keyword If Test Failed  Log File  ${SUPPLEMENTARY_FILE}

    Remove File  ${STATUS_FILE}
    Remove File  ${TELEMETRY_STATUS_FILE}
    Remove File  ${EXE_CONFIG_FILE}
    Remove File  ${TELEMETRY_OUTPUT_JSON}

    Restore Moved Files
    Run Process  mv  ${TELEMETRY_EXECUTABLE}.orig  ${TELEMETRY_EXECUTABLE}
    Terminate All Processes  kill=True
    Cleanup Temporary Folders

*** Test Cases ***
Telemetry Runs With Values From Configuration File
    [Documentation]  Telemetry Scheduler runs with values from the configuration (supplementary) file

    ${time} =  Get Current Time
    Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}
    Check Telemetry Scheduler Is Running


Diagnose Collects Telemetry Logs And Config Files
    ${TarTempDir} =  Add Temporary Directory  tarTempdir
    # replace real telemetry executable
    Run Process  cp  ${TELEMETRY_EXECUTABLE}.orig  ${TELEMETRY_EXECUTABLE}
    Wait For Telemetry Executable To Have Run

    File Should Exist 	${SUPPLEMENTARY_FILE}
    File Should Exist 	${STATUS_FILE}              #Generated when the telemetry scheduler runs
    File Should Exist 	${TELEMETRY_SCHEDULER_LOG}
    File Should Exist 	${EXE_CONFIG_FILE}          #Generated when the telemetry executable runs
    File Should Exist 	${TELEMETRY_OUTPUT_JSON}    #Generated when the telemetry executable runs
    File Should Exist 	${TELEMETRY_EXECUTABLE_LOG}

    ${result} =  Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose  ${TarTempDir}
    Should Be Equal As Integers  ${result.rc}  0
    ${Files} =  List Files In Directory  ${TarTempDir}
    ${LengthOfFiles} =  Get Length  ${Files}
    should Be Equal As Numbers  1  ${LengthOfFiles}
    ${TarContents} =  Query Tarfile For Contents  ${TarTempDir}/${Files[0]}

    Should Contain 	${TarContents}  BaseFiles/telemetry-config.json
    Should Contain 	${TarContents}  BaseFiles/tscheduler-status.json
    Should Contain 	${TarContents}  BaseFiles/tscheduler.log
    Should Contain 	${TarContents}  BaseFiles/telemetry-exe.json
    Should Contain 	${TarContents}  BaseFiles/telemetry.json
    Should Contain 	${TarContents}  BaseFiles/telemetry.log


Telemetry Scheduler Schedules Telemetry Executable
    [Documentation]  Telemetry is scheduled and launches telemetry executable with a configuration file that passes all the required details to send telemetry data

    ${time} =  Get Current Time
    Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

    Wait For Telemetry Executable To Have Run
    Check Telemetry Scheduler Is Running

Telemetry Scheduler Schedules Telemetry Executable for ten minutues on a fresh install
    [Documentation]  Telemetry is scheduled and launches telemetry executable in ten minutes on a fresh install where the status file is not present
    Remove File  ${STATUS_FILE}
    Restart Telemetry Scheduler

    ${time} =  Get Current Time
    Check Scheduled Time For Fresh installs    ${time}

Telemetry Scheduler recives error output from telemetry executable when it fails
    [Setup]  Telemetry Scheduler Plugin Test Setup with error telemetry executable

    # Would exit with correct exit code but just wouldn't show the right message in logs
    Wait Until Keyword Succeeds
    ...  100s
    ...  10s
    ...  Check Tscheduler Log Contains  Telemetry executable failed with exit code 255

Telemetry Scheduler Generates Files With Correct Permissions
    [Documentation]  Telemetry is scheduled and launches telemetry executable, generating files with the correct permissions

    Wait For Telemetry Executable To Have Run

    File Exists With Permissions  ${STATUS_FILE}             sophos-spl-user   sophos-spl-group  -rw-------
    File Exists With Permissions  ${EXE_CONFIG_FILE}         sophos-spl-user   sophos-spl-group  -rw-------
    File Exists With Permissions  ${TELEMETRY_OUTPUT_JSON}   sophos-spl-user   sophos-spl-group  -rw-------

Telemetry Scheduler Schedules Telemetry Executable Multiple Times
    [Documentation]  Telemetry is scheduled and launchs telemetry executable more than once

    ${time} =  Get Current Time
    Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

    Wait For Telemetry Executable To Have Run

    ${time} =  Get Current Time
    Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

    Wait For Telemetry Executable To Have Run
    Check Telemetry Scheduler Is Running


Telemetry Scheduler Picks Up Interval Change
    [Documentation]  Changes in the interval value from the configuration (supplementary) file are picked up when status file is written

    ${time} =  Get Current Time
    Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

    ${new_interval} =  Set Variable  86400
    Set Interval In Configuration File  ${new_interval}

    Wait For Telemetry Executable To Have Run
    ${time} =  Get Current Time

    Wait Until Keyword Succeeds  ${TELEMETRY_EXE_CHECK_DELAY} seconds  1 seconds   Check Scheduled Time Against Telemetry Config Interval  ${time}  ${new_interval}
    Check Telemetry Scheduler Is Running


Telemetry Scheduler Runs Executable On StartUp If Status Time Is In Past
    [Documentation]  Telemetry will collect on startup if its scheduled time is in the past

    ${next_day_interval} =  Set Variable  86400
    Set Interval In Configuration File  ${next_day_interval}

    ${time} =  Get Current Time
    ${scheduled_time_in_past} =  Expected Scheduled Time  ${time}  -5
    Set Scheduled Time In Scheduler Status File   ${scheduled_time_in_past}
    Restart Telemetry Scheduler

    Wait For Telemetry Executable To Have Run
    ${time} =  Get Current Time

    Wait Until Keyword Succeeds  ${TELEMETRY_EXE_CHECK_DELAY} seconds  1 seconds   Check Scheduled Time Against Telemetry Config Interval  ${time}  ${next_day_interval}
    Check Telemetry Scheduler Is Running


Telemetry Scheduler Does Not Reschedule If Status Time Is In Future
    [Documentation]  Telemetry will delay for the appropriate time on startup if the scheduled time is in the future
    ${scheduled_time_before_restart} =  Check Status File Is Generated And Return Scheduled Time

    Restart Telemetry Scheduler

    ${scheduled_time_after_restart} =  Check Status File Is Generated And Return Scheduled Time
    Should Be Equal As Integers  ${scheduled_time_before_restart}  ${scheduled_time_after_restart}  The scheduled time in the future must not be changed by scheduler restart
    Check Telemetry Scheduler Is Running

Telemetry Scheduler Recovers From Broken Status File
    [Documentation]  Telemetry will recover by replacing a broken status file

    Break Scheduler Status File

    Restart Telemetry Scheduler
    ${time} =  Get Current Time
    Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

    Wait For Telemetry Executable To Have Run
    Check Telemetry Scheduler Is Running


Telemetry Scheduler Recovers From Failure To Launch Telemetry Executable
    [Documentation]  Telemetry will recover from a failure to launch telemetry executable

    Remove File  ${STATUS_FILE}
    Create File  ${STATUS_FILE}     {"scheduled-time":1682199662}
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${STATUS_FILE}    shell=True
    Remove File  ${TELEMETRY_OUTPUT_JSON}
    ${ts_log}=    mark_log_size    ${TELEMETRY_SCHEDULER_LOG}
    Move File From Expected Path  ${TELEMETRY_EXECUTABLE}
    File Should Not Exist  ${TELEMETRY_OUTPUT_JSON}

    Sleep  ${TEST_INTERVAL} seconds  # wait until telemetry executable run time
    Wait Until Keyword Succeeds  ${TIMING_TOLERANCE} seconds  1 seconds   File Should Exist  ${EXE_CONFIG_FILE}

    Restore Moved Files

    Sleep  ${TELEMETRY_EXE_CHECK_DELAY} seconds  # wait for telemetry scheduler to check executable
    wait_for_log_contains_from_mark    ${ts_log}    Failure to start process: execve failed: No such file or directory    15
    ${time} =  Get Current Time
    Wait Until Keyword Succeeds
    ...     ${TIMING_TOLERANCE} seconds
    ...     1 seconds
    ...     Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

    Wait For Telemetry Executable To Have Run
    Check Telemetry Scheduler Is Running

Telemetry Scheduling Can Be Disabled Explicitly Via Configuration File
    [Documentation]  Scheduling can be disabled remotely

    Remove File  ${STATUS_FILE}
    Set Interval In Configuration File  0
    Restart Telemetry Scheduler

    Sleep  ${TIMING_TOLERANCE} seconds  # allow time to schedule a telemetry run if telemetry was enabled
    File Should Not Exist  ${STATUS_FILE}

Telemetry Scheduling Can Be Disabled Explicitly Via Status File
    [Documentation]  Scheduling can be disabled locally

    Set Scheduled Time In Scheduler Status File  0
    Restart Telemetry Scheduler
    Remove File  ${EXE_CONFIG_FILE}

    # sleep until after file would be written if telemetry was enabled
    Sleep  ${TEST_INTERVAL}
    Sleep  ${TIMING_TOLERANCE} seconds

    File Should Not Exist  ${EXE_CONFIG_FILE}

Telemetry Scheduling Is Disabled If Configuration File Is Missing
    [Documentation]  Scheduling is disabled if configuration file is missing

    Remove File  ${STATUS_FILE}
    Move File From Expected Path  ${SUPPLEMENTARY_FILE}
    Restart Telemetry Scheduler

    Sleep  ${TIMING_TOLERANCE} seconds  # allow time to schedule a telemetry run if telemetry was enabled
    File Should Not Exist  ${STATUS_FILE}

    Restore Moved Files
    Check Telemetry Scheduler Is Running

Telemetry Scheduling Is Disabled If Configuration File Is Invalid
    [Documentation]  Scheduling is disabled if configuration (supplementary) file is invalid

    Remove File  ${STATUS_FILE}
    Move File From Expected Path  ${SUPPLEMENTARY_FILE}
    Set Invalid Json In Configuration File
    Restart Telemetry Scheduler

    Sleep  ${TIMING_TOLERANCE} seconds  # allow time to schedule a telemetry run if telemetry was enabled
    File Should Not Exist  ${STATUS_FILE}

    Restore Moved Files
    Check Telemetry Scheduler Is Running


Telemetry Sending Requires Correct Headers In Configuration File
    Move File From Expected Path  ${SUPPLEMENTARY_FILE}
    Set Invalid Additional Headers In Configuration File
    Check Telemetry Scheduler Is Running


Telemetry Scheduler Terminates Executable When Timeout Is Exceeded
    [Documentation]  The scheduler terminates the telemetry executable when it exceeds the timeout and logs an error

    #Telemetry executable takes longer than the timeout
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    sleep 70s
    ...    \

    Create File   ${TELEMETRY_EXECUTABLE}    content=${script}
    Run Process  chmod  ug+x  ${TELEMETRY_EXECUTABLE}
    Run Process  chgrp  sophos-spl-group  ${TELEMETRY_EXECUTABLE}

    Check Telemetry Scheduler Is Running

    Wait Until Keyword Succeeds  ${TEST_INTERVAL} seconds  1 seconds   File Should Exist  ${EXE_CONFIG_FILE}

    Sleep  ${TIMING_TOLERANCE} seconds

    Check Telemetry Executable Is Running
    ${telemetry_pid} =   Run Process   pgrep  -f   telemetry

    #Timeout error is logged and the telemetry executable is terminated
    Wait Until Keyword Succeeds  ${TELEMETRY_EXE_CHECK_DELAY}  10 seconds   Check Log Contains   Running telemetry executable timed out    ${TELEMETRY_SCHEDULER_LOG}    tscheduler.log

    ${result} =    Run Process    ps --pid ${telemetry_pid.stdout}    shell=true
    Should Not Be Equal As Integers   ${result.rc}    0

Telemetry Scheduler Starts And Does Not Run Executable As No Policy Is Received
    [Setup]    Telemetry Scheduler Plugin Test Setup Without Sending Policy

    wait until keyword succeeds    ${TELEMETRY_EXE_CHECK_DELAY}    1 seconds    file should not exist    ${TELEMETRY_OUTPUT_JSON}

Telemetry Scheduler Starts, Receives Policy And Runs Executable Then Receives A Different Policy And Processes It WIthout Errors
    # Receving default policy and running telemetry
    wait until created    ${TELEMETRY_OUTPUT_JSON}
    Remove File    ${TELEMETRY_OUTPUT_JSON}

    # Receiving different policy and checking
    Simulate Send Policy    ALC_policy_with_telemetry_hostname.xml

    wait until created    ${TELEMETRY_OUTPUT_JSON}    timeout=5 minutes

    # Hostname received from new policy should be: test.sophusupd.net (telemetry hostname found in ALC_policy_with_telemetry_hostname.xml)
    check tscheduler log contains    test.sophosupd.net

Telemetry Scheduler Recovers From Failure To Launch Telemetry Executable With Last Start Field
    [Documentation]  Telemetry will recover from a failure to launch telemetry executable

    Remove File  ${STATUS_FILE}
    Create File  ${STATUS_FILE}     {"scheduled-time":1682199662, "last-start-time":1682199662}
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${STATUS_FILE}    shell=True
    Remove File  ${TELEMETRY_OUTPUT_JSON}
    ${ts_log}=    mark_log_size    ${TELEMETRY_SCHEDULER_LOG}
    Move File From Expected Path  ${TELEMETRY_EXECUTABLE}
    File Should Not Exist  ${TELEMETRY_OUTPUT_JSON}

    Sleep  ${TEST_INTERVAL} seconds  # wait until telemetry executable run time
    Wait Until Keyword Succeeds  ${TIMING_TOLERANCE} seconds  1 seconds   File Should Exist  ${EXE_CONFIG_FILE}

    Restore Moved Files

    Sleep  ${TELEMETRY_EXE_CHECK_DELAY} seconds  # wait for telemetry scheduler to check executable
    wait_for_log_contains_from_mark    ${ts_log}    Failure to start process: execve failed: No such file or directory    15
    ${time} =  Get Current Time
    Wait Until Keyword Succeeds
    ...     ${TIMING_TOLERANCE} seconds
    ...     1 seconds
    ...     Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

    Wait For Telemetry Executable To Have Run
    Check Telemetry Scheduler Is Running