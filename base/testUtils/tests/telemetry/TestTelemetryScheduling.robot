*** Settings ***
Documentation  Test the Telemetry Scheduler

Library    ${libs_directory}/TemporaryDirectoryManager.py

Resource  TelemetryResources.robot
Resource  ../GeneralTeardownResource.robot

Suite Setup      Setup TelemetryScheduling Tests
Suite Teardown   Cleanup TelemetryScheduling Tests

Test Setup       Telemetry Scheduler Plugin Test Setup
Test Teardown    Telemetry Scheduler Plugin Test Teardown

Default Tags  TELEMETRY SCHEDULER

*** Keywords ***
### Suite Setup
Setup TelemetryScheduling Tests
    Require Fresh Install
    Copy File  ${TELEMETRY_CONFIG_FILE_SOURCE}  ${TELEMETRY_CONFIG_FILE}


### Suite Cleanup
Cleanup TelemetryScheduling Tests
    Uninstall SSPL


Telemetry Scheduler Plugin Test Setup
    Run Process  mv  ${TELEMETRY_EXECUTABLE}  ${TELEMETRY_EXECUTABLE}.orig
    Create Fake Telemetry Executable
    Set Interval In Configuration File  ${TEST_INTERVAL}
    Remove File  ${STATUS_FILE}
    Restart Telemetry Scheduler
    Wait Until Keyword Succeeds  20 seconds  1 seconds   File Should Exist  ${STATUS_FILE}


Telemetry Scheduler Plugin Test Teardown
    General Test Teardown
    Run Keyword If Test Failed  Log File  ${SUPPLEMENTARY_FILE}

    Remove File  ${STATUS_FILE}
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
    Remove File  ${TELEMETRY_OUTPUT_JSON}

    Move File From Expected Path  ${TELEMETRY_EXECUTABLE}
    File Should Not Exist  ${STATUS_FILE}
    File Should Not Exist  ${TELEMETRY_OUTPUT_JSON}

    Sleep  ${TEST_INTERVAL} seconds  # wait until telemetry executable run time
    Wait Until Keyword Succeeds  ${TIMING_TOLERANCE} seconds  1 seconds   File Should Exist  ${EXE_CONFIG_FILE}

    Restore Moved Files

    Sleep  ${TELEMETRY_EXE_CHECK_DELAY} seconds  # wait for telemetry scheduler to check executable
    Wait Until Keyword Succeeds  ${TIMING_TOLERANCE} seconds  1 seconds   File Should Exist  ${STATUS_FILE}
    ${time} =  Get Current Time
    Check Scheduled Time Against Telemetry Config Interval  ${time}  ${TEST_INTERVAL}

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

    #Timeout error is logged and the telemetry executable is terminated
    Wait Until Keyword Succeeds  ${TELEMETRY_EXE_CHECK_DELAY}  10 seconds   Check Log Contains   Running telemetry executable timed out    ${TELEMETRY_SCHEDULER_LOG}    tscheduler.log

    Check Telemetry Executable Is Not Running