*** Settings ***
Documentation    Telemetry testing resources.

Library    OperatingSystem
Library    Process
Library    String
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/HttpsServer.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py
Library    ${LIBS_DIRECTORY}/PolicyUtils.py
Library    ${LIBS_DIRECTORY}/TelemetryUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py

Resource    InstallerResources.robot
Resource    WatchdogResources.robot

*** Variables ***
${STATUS_FILE}  ${SOPHOS_INSTALL}/base/telemetry/var/tscheduler-status.json
${SUPPLEMENTARY_FILE}   ${SOPHOS_INSTALL}/base/etc/telemetry-config.json
${EXE_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/telemetry/var/telemetry-exe.json
${TELEMETRY_OUTPUT_JSON}    ${SOPHOS_INSTALL}/base/telemetry/var/telemetry.json

${TELEMETRY_CONFIG_FILE_SOURCE}  ${SUPPORT_FILES}/Telemetry/telemetry-config.json
${TELEMETRY_CONFIG_FILE}   ${SOPHOS_INSTALL}/base/etc/telemetry-config.json

${TELEMETRY_EXECUTABLE}    ${SOPHOS_INSTALL}/base/bin/telemetry
${TEST_INTERVAL}  16
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
${USERNAME}    sophos-spl-user

${MACHINE_ID_FILE}  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
${tmpPolicy}       /tmp/tmpALC.xml

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
    ...    exit 255
    ...    \
    Create File   ${TELEMETRY_EXECUTABLE}    content=${script}
    Run Process  chmod  ug+x  ${TELEMETRY_EXECUTABLE}
    Run Process  chgrp  sophos-spl-group  ${TELEMETRY_EXECUTABLE}

Prepare To Run Telemetry Executable
    Prepare To Run Telemetry Executable With HTTPS Protocol

Prepare To Run Telemetry Executable With Broken Put Requests
    Set Environment Variable  BREAK_PUT_REQUEST  TRUE
    Prepare To Run Telemetry Executable With HTTPS Protocol

Prepare To Run Telemetry Executable With HTTPS Protocol
    [Arguments]  ${port}=443  ${TLSProtocol}=tlsv1_2
    HttpsServer.Start Https Server  ${CERT_PATH}  ${port}  ${TLSProtocol}
    Wait Until Keyword Succeeds  10 seconds  1.0 seconds  File Should Exist  ${MACHINE_ID_FILE}
    Create Test Telemetry Config File  ${EXE_CONFIG_FILE}  ${CERT_PATH}  ${USERNAME}  port=${port}

Prepare To Run Telemetry Executable That Hangs
    [Arguments]  ${port}=443  ${TLSProtocol}=tlsv1_2
    HttpsServer.Start Https Server  ${CERT_PATH}  ${port}  ${TLSProtocol}
    Wait Until Keyword Succeeds  10 seconds  1.0 seconds  File Should Exist  ${MACHINE_ID_FILE}
    Create Test Telemetry Config File  ${EXE_CONFIG_FILE}  ${CERT_PATH}  ${USERNAME}  port=${port} server=

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

Run Telemetry Executable That Hangs
    [Arguments]  ${telemetryConfigFilePath}

    Remove File  ${TELEMETRY_EXECUTABLE_LOG}

    ${result} =  Start Process  sudo  -u  ${USERNAME}  ${SOPHOS_INSTALL}/base/bin/telemetry  ${telemetryConfigFilePath}

Kill Telemetry If Running
    ${result} =    Run Process  pgrep  telemetry
    Log  ${result.stdout}
    Run Keyword If  ${result.rc} == 0  Run Process  kill   ${result.stdout}

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
    [Arguments]  ${start_time}  ${test_interval}  ${tolerance}=600
    ${actual_interval} =  Get Interval From Supplementary File
    Should Be Equal As Integers  ${actual_interval}  ${test_interval}  The interval in the telemetry-config.json file is different from the expected test interval

    ${scheduled_time} =  Check Status File Is Generated And Return Scheduled Time
    ${expected_scheduled_time} =  Expected Scheduled Time  ${start_time}  ${test_interval}

    ${is_within_range} =  Verify Scheduled Time Is Expected  ${scheduled_time}  ${expected_scheduled_time}  ${tolerance}
    Should Be True  ${is_within_range}  msg="Actual scheduled time ${scheduled_time} is expected to be within +/- ${tolerance} seconds of ${expected_scheduled_time}"


Check Scheduled Time For Fresh installs
    [Arguments]  ${start_time}  ${tolerance}=30

    ${scheduled_time} =  Check Status File Is Generated And Return Scheduled Time
    ${expected_scheduled_time} =  Expected Scheduled Time  ${start_time}  600

    ${is_within_range} =  Verify Scheduled Time Is Expected  ${scheduled_time}  ${expected_scheduled_time}  ${tolerance}
    Should Be True  ${is_within_range}  msg="Actual scheduled time ${scheduled_time} is expected to be within +/- ${tolerance} seconds of ${expected_scheduled_time}"


Cleanup Telemetry Server
    Stop Https Server
    Dump Teardown Log  ${HTTPS_LOG_FILE_PATH}
    Remove File  ${HTTPS_LOG_FILE_PATH}

Cleanup Telemetry Server And Remove Telemetry Output
    Cleanup Telemetry Server
    Remove File  ${TELEMETRY_OUTPUT_JSON}

Copy Telemetry Config File in To Place
    Copy File  ${TELEMETRY_CONFIG_FILE_SOURCE}  ${TELEMETRY_CONFIG_FILE}

Drop sophos-spl-local File Into Place
    [Arguments]  ${sourceFilePath}  ${destFilepath}
    Copy File   ${sourceFilePath}   ${SOPHOS_INSTALL}/tmp/tempFile
    Run Process  chmod  640  ${SOPHOS_INSTALL}/tmp/tempFile
    Run Process  chown  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/tmp/tempFile
    Move File   ${SOPHOS_INSTALL}/tmp/tempFile  ${destFilepath}
    File Exists With Permissions   ${destFilepath}   sophos-spl-local  sophos-spl-group  -rw-r-----

Drop ALC Policy Into Place
    Drop sophos-spl-local File Into Place     ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Drop ALC Policy With Fixed Version Into Place
    ${paused_updating_ALC} =    populate_cloud_subscription_with_paused
    Create File  ${tmpPolicy}   ${paused_updating_ALC}
    Register Cleanup    Remove File    ${tmpPolicy}
    Drop sophos-spl-local File Into Place     ${tmpPolicy}  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Drop ALC Policy With ESM Into Place
    [Arguments]    ${esmname}    ${esmtoken}
    ${esm_ALC} =    populate_fixed_version_with_normal_cloud_sub    ${esmname}    ${esmtoken}
    Create File  ${tmpPolicy}   ${esm_ALC}
    Register Cleanup    Remove File    ${tmpPolicy}
    Drop sophos-spl-local File Into Place     ${tmpPolicy}  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Drop ALC Policy With Scheduled Updating Into Place
    Drop sophos-spl-local File Into Place     ${SUPPORT_FILES}/CentralXml/ALC_policy_delayed_updating.xml  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Drop MCS Config Into Place
    Drop sophos-spl-local File Into Place     ${SUPPORT_FILES}/base_data/mcs.config  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

Drop LiveQuery Policy Into Place
    Drop sophos-spl-local File Into Place     ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_LiveQuery_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy/LiveQuery-1_policy.xml
