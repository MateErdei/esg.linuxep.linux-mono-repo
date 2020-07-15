*** Settings ***
Documentation    Telemetry testing resources.

Library    OperatingSystem
Library    Process
Library    String
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/TelemetryUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/HttpsServer.py

Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/WatchdogResources.robot

*** Variables ***
${STATUS_FILE}  ${SOPHOS_INSTALL}/base/telemetry/var/tscheduler-status.json
${SUPPLEMENTARY_FILE}   ${SOPHOS_INSTALL}/base/etc/telemetry-config.json
${EXE_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/telemetry/var/telemetry-exe.json
${TELEMETRY_OUTPUT_JSON}    ${SOPHOS_INSTALL}/base/telemetry/var/telemetry.json

${TELEMETRY_CONFIG_FILE_SOURCE}  ${SUPPORT_FILES}/Telemetry/telemetry-config.json
${TELEMETRY_CONFIG_FILE}   ${SOPHOS_INSTALL}/base/etc/telemetry-config.json

${TELEMETRY_EXECUTABLE}    ${SOPHOS_INSTALL}/base/bin/telemetry
${TEST_INTERVAL}  15
${TELEMETRY_EXE_CHECK_DELAY}  60  # this should match the scheduler's implementation
${TIMING_TOLERANCE}  10

${SUCCESS}    0
${FAILED}     1

${TELEMETRY_EXECUTABLE_LOG}    ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log
${TELEMETRY_SCHEDULER_LOG}   ${SOPHOS_INSTALL}/logs/base/sophosspl/tscheduler.log
${HTTPS_LOG_FILE}     https_server.log
${HTTPS_LOG_FILE_PATH}     /tmp/${HTTPS_LOG_FILE}

${SERVER}      localhost
${CERT_PATH}   /tmp/cert.pem
${USERNAME}     sophos-spl-user

${MACHINE_ID_FILE}  ${SOPHOS_INSTALL}/base/etc/machine_id.txt


*** Keywords ***
Create Fake Telemetry Executable
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo '{"system-telemetry":{"os-name":"Ubuntu","os-version":"18.04"}}' > ${TELEMETRY_OUTPUT_JSON}
    ...    \
    Create File   ${TELEMETRY_EXECUTABLE}    content=${script}
    Run Process  chmod  ug+x  ${TELEMETRY_EXECUTABLE}
    Run Process  chgrp  sophos-spl-group  ${TELEMETRY_EXECUTABLE}

Create Fake Telemetry Executable that exits with error
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo 'error'
    ...    exit 1
    ...    \
    Create File   ${TELEMETRY_EXECUTABLE}    content=${script}
    Run Process  chmod  ug+x  ${TELEMETRY_EXECUTABLE}
    Run Process  chgrp  sophos-spl-group  ${TELEMETRY_EXECUTABLE}

Prepare To Run Telemetry Executable
    Prepare To Run Telemetry Executable With HTTPS Protocol

Prepare To Run Telemetry Executable With HTTPS Protocol
    [Arguments]  ${port}=443  ${TLSProtocol}=tlsv1_2
    HttpsServer.Start Https Server  ${CERT_PATH}  ${port}  ${TLSProtocol}
    Wait Until Keyword Succeeds  10 seconds  1.0 seconds  File Should Exist  ${MACHINE_ID_FILE}
    Create Test Telemetry Config File  ${EXE_CONFIG_FILE}  ${CERT_PATH}  ${USERNAME}  port=${port}

Check Telemetry Log Contains
    [Arguments]  ${CONTENT}
    Check Log Contains   ${CONTENT}   ${TELEMETRY_EXECUTABLE_LOG}   telemetry.log

Check Telemetry Content
    ${telemetryFileContents} =  Get File  ${TELEMETRY_OUTPUT_JSON}

    Should Exist  ${TELEMETRY_EXECUTABLE_LOG}
    File Should Not Be Empty  ${TELEMETRY_EXECUTABLE_LOG}
    Check Log Contains  ${telemetryFileContents}  ${HTTPS_LOG_FILE_PATH}  ${HTTPS_LOG_FILE}

Run Telemetry Executable
    [Arguments]  ${telemetryConfigFilePath}  ${expectedResult}   ${checkResult}=1

    Remove File  ${TELEMETRY_EXECUTABLE_LOG}

    ${result} =  Run Process  sudo  -u  ${USERNAME}  ${SOPHOS_INSTALL}/base/bin/telemetry  ${telemetryConfigFilePath}

    Log  "stdout = ${result.stdout}"
    Log  "stderr = ${result.stderr}"

    Should Be Equal As Integers  ${result.rc}  ${expectedResult}  Telemetry executable returned a non-successful error code

    Run Keyword If   ${checkResult}==1  Check Telemetry Content



Wait For Telemetry Executable To Have Run
    Log  Waiting for telemetry executable to run...
    Sleep  ${TIMING_TOLERANCE} seconds
    Wait Until Keyword Succeeds  ${TEST_INTERVAL} seconds  1 seconds   File Should Exist  ${EXE_CONFIG_FILE}
    Wait Until Keyword Succeeds  ${TELEMETRY_EXE_CHECK_DELAY} seconds  1 seconds   File Should Exist  ${TELEMETRY_OUTPUT_JSON}
    Log  Telemetry executable has started


Restart Telemetry Scheduler
    Stop Telemetry Scheduler
    Start Telemetry Scheduler
    Wait Until Keyword Succeeds  ${TIMING_TOLERANCE}  1 seconds   Check Telemetry Scheduler Is Running


Stop Telemetry Scheduler
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   tscheduler
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers  ${result.rc}  0  msg="Failed to stop telemetry scheduler ${result.stdout} ${result.stderr}"
    Wait Until Keyword Succeeds  20 seconds    1 seconds   Check Telemetry Scheduler Plugin Not Running


Start Telemetry Scheduler
    ${wdctl} =   Set Variable  ${SOPHOS_INSTALL}/bin/wdctl
    ${result} =    Run Process    ${wdctl}    start  tscheduler
    Should Be Equal As Integers  ${result.rc}  0  msg="Failed to start telemetry scheduler ${result.stdout} ${result.stderr}"
    Wait Until Keyword Succeeds  20 seconds  1 seconds   Check Telemetry Scheduler Is Running


Check Telemetry Executable Is Running
    ${result} =    Run Process  pgrep  telemetry
    Should Be Equal As Integers    ${result.rc}    0


Check Telemetry Executable Is Not Running
    ${result} =    Run Process  pgrep  telemetry
    Should Not Be Equal As Integers    ${result.rc}    0

Check Scheduled Time Against Telemetry Config Interval
    [Arguments]  ${start_time}  ${test_interval}  ${tolerance}=30
    ${actual_interval} =  Get Interval From Supplementary File
    Should Be Equal As Integers  ${actual_interval}  ${test_interval}  The interval in the telemetry-config.json file is different from the expected test interval

    ${scheduled_time} =  Check Status File Is Generated And Return Scheduled Time
    ${expected_scheduled_time} =  Expected Scheduled Time  ${start_time}  ${test_interval}

    ${is_within_range} =  Verify Scheduled Time Is Expected  ${scheduled_time}  ${expected_scheduled_time}  ${tolerance}
    Should Be True  ${is_within_range}  msg="Actual scheduled time ${scheduled_time} is expected to be within +/- ${tolerance} seconds of ${expected_scheduled_time}"


Cleanup Telemetry Server
    Stop Https Server
    Log  ${HTTPS_LOG_FILE_PATH}
    Remove File  ${HTTPS_LOG_FILE_PATH}

Copy Telemetry Config File in To Place
    Copy File  ${TELEMETRY_CONFIG_FILE_SOURCE}  ${TELEMETRY_CONFIG_FILE}

Drop sophos-spl-user File Into Place
    [Arguments]  ${sourceFilePath}  ${destFilepath}
    Copy File   ${sourceFilePath}   ${destFilepath}
    Run Process  chmod  600  ${destFilepath}
    Run Process  chown  sophos-spl-user:sophos-spl-group  ${destFilepath}
    File Exists With Permissions   ${destFilepath}   sophos-spl-user  sophos-spl-group  -rw-------

Drop ALC Policy Into Place
    Drop sophos-spl-user File Into Place     ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Drop ALC Policy With Fixed Version Into Place
    Drop sophos-spl-user File Into Place     ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicy.xml  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Drop MCS Config Into Place
    Drop sophos-spl-user File Into Place     ${SUPPORT_FILES}/base_data/mcs.config  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
