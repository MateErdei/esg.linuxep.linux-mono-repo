*** Settings ***
Documentation     Test the Telemetry executable

Library    OperatingSystem

Library    ${LIBS_DIRECTORY}/ActionUtils.py
Library    ${LIBS_DIRECTORY}/CentralUtils.py
Library    ${LIBS_DIRECTORY}/EventUtils.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OnFail.py
Library    ${LIBS_DIRECTORY}/SystemInfo.py

Resource    TelemetryResources.robot

Resource    ../GeneralTeardownResource.robot
Resource    ../installer/InstallerResources.robot
Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot

Suite Setup      Setup Telemetry Tests
Suite Teardown   Cleanup Telemetry Tests

Test Setup       Telemetry Test Setup
Test Teardown    Telemetry Test Teardown

Force Tags    TELEMETRY    TAP_PARALLEL1


*** Variables ***
${MANAGEMENT_AGENT_LOG}         ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log

*** Keywords ***
### Suite Setup
Setup Telemetry Tests
    Regenerate Certificates
    Start Local Cloud Server
    Require Fresh Install
    Set Local CA Environment Variable
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Copy Telemetry Config File in To Place


### Suite Cleanup
Cleanup Telemetry Tests
    Uninstall SSPL
    Stop Local Cloud Server
    Unset CA Environment Variable
    Cleanup Certificates

### Test setup

Create Empty SulDownloader Config
    Create File    ${UPDATE_CONFIG}

Telemetry Test Setup
    Prepare To Run Telemetry Executable


Telemetry Test Setup With Broken Put Requests
    Prepare To Run Telemetry Executable With Broken Put Requests


Telemetry Test Teardown
    Run Keyword If Test Failed   Dump Cloud Server Log
    Remove Environment Variable  https_proxy
    Stop Proxy Servers
    Stop Proxy If Running
    Kill Telemetry If Running
    Reset MachineID Permissions
    General Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Restore System Commands
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Reset MachineID Permissions
    Remove File   ${UPDATE_CONFIG}
    Remove File   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Remove Environment Variable  BREAK_PUT_REQUEST


Teardown With Proxy Clear
    Remove File   /opt/sophos-spl/base/etc/sophosspl/current_proxy
    Telemetry Test Teardown

Stop Fake Plugin Teardown
    Stop Plugin
    Stop Management Agent
    Telemetry Test Teardown

Reset MachineID Permissions
    Run Process  chmod  640  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/machine_id.txt

Run Telemetry And Expect Field Missing
    [Arguments]   ${missing_field}
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${SUCCESS}
    ${telemetry_file_contents} =  Get File  ${TELEMETRY_OUTPUT_JSON}
    # Both test generated and product generated telemetry fill in the over all health field by reading
    # the SHS status file, if this file changes between the reads then there can be a mismatch and the test fails.
    # As the tests using this keyword have nothing to do with health we're ignoring that telemetry field.
    Check Base Telemetry Json Is Correct  ${telemetry_file_contents}  expected_missing_keys=${missing_field}  ignored_fields=overall-health


*** Test Cases ***
Telemetry Executable Generates System Base and Watchdog Telemetry
    [Tags]  SMOKE  TELEMETRY
    [Documentation]    Telemetry Executable Generates Telemetry

    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG}  Starting service health checks  timeout=${120}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Should contain  ${telemetryFileContents}  "sdu":{"health":0}
    Should contain  ${telemetryFileContents}  "tscheduler":{"health":0}
    Check System Telemetry Json Is Correct  ${telemetryFileContents}
    Check Watchdog Telemetry Json Is Correct  ${telemetryFileContents}
    Check Base Telemetry Json Is Correct  ${telemetryFileContents}

Telemetry Executable Generates Cloud Platform Metadata
    [Tags]  SMOKE  TELEMETRY  AMAZON_LINUX
    [Documentation]    Telemetry Executable Generates Telemetry
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check System Telemetry Json Is Correct  ${telemetryFileContents}
    File Should Contain  ${TELEMETRY_OUTPUT_JSON}  "cloud-platform":"AWS"

Telemetry Executable Generates mcs-connection when message relay
    [Tags]  SMOKE  TELEMETRY
    [Teardown]  Teardown With Proxy Clear
    Start Simple Proxy Server    3000
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    File Should Contain  ${TELEMETRY_EXECUTABLE_LOG}  Uploading via proxy: localhost:3000
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    File Should Contain  ${TELEMETRY_OUTPUT_JSON}  "mcs-connection":"Message Relay"

Telemetry Executable Generates mcs-connection when proxy
    [Tags]  SMOKE  TELEMETRY
    [Teardown]  Teardown With Proxy Clear
    Start Simple Proxy Server    3001
    Set Environment Variable  https_proxy   http://localhost:3001
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3001

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    File Should Contain  ${TELEMETRY_OUTPUT_JSON}  "mcs-connection":"Proxy"
    File Should Contain  ${TELEMETRY_EXECUTABLE_LOG}  Uploading via proxy: localhost:3001


Telemetry Executable Generates Update Scheduler Telemetry
    # Make sure there are no left over update telemetry items.
    Cleanup Telemetry Server
    Require Fresh Install

    Create Empty SulDownloader Config
    setup mcs config with JWT token
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Drop ALC Policy Into Place
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Processing Flags    ${10}

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  0
    ...    set_edr=True
    ...    set_av=True
    ...    install_state=0
    ...    download_state=0

Telemetry Executable Generates Update Scheduler Telemetry With Fixed Version And Tag In Policy
    # Make sure there are no left over update telemetry items.
    Cleanup Telemetry Server
    Require Fresh Install

    Create Empty SulDownloader Config
    setup mcs config with JWT token
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Drop ALC Policy With Fixed Version Into Place
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Processing Flags    ${10}

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}   0
    ...    base_fixed_version=2022.1.0.40
    ...    install_state=0
    ...    download_state=0

Telemetry Executable Generates Update Scheduler Telemetry With Scheduled Updating
    # Make sure there are no left over update telemetry items.
    Cleanup Telemetry Server
    Require Fresh Install

    Create Empty SulDownloader Config
    setup mcs config with JWT token
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Drop ALC Policy With Scheduled Updating Into Place
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Processing Flags    ${10}
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Scheduling product updates for Sunday at 12:00    ${10}

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}   0
    ...    scheduled_updating_enabled=${True}
    ...    scheduled_updating_day=Sunday
    ...    scheduled_updating_time=12:00
    ...    install_state=0
    ...    download_state=0

Telemetry Executable Generates Update Scheduler Telemetry With ESM Set In Policy
    ${esmname} =  Set Variable   LTS 2023.1.1
    # Make sure there are no left over update telemetry items.
    Cleanup Telemetry Server
    Require Fresh Install

    Create Empty SulDownloader Config
    setup mcs config with JWT token
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Drop ALC Policy With ESM Into Place     ${esmname}    f4d41a16-b751-4195-a7b2-1f109d49469d
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Processing Flags    ${10}
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}   0
    ...    base_fixed_version=${esmname}
    ...    install_state=0
    ...    download_state=0


Telemetry Executable Log Has Correct Permissions
    [Documentation]    Telemetry Executable Log Has Correct Permissions
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    File Exists With Permissions  ${TELEMETRY_EXECUTABLE_LOG}  sophos-spl-user  sophos-spl-group  -rw-------

Telemetry Executable Creates HTTP POST Request
    [Documentation]    Telemetry Executable Creates HTTP POST Request With Collected Telemetry Content
    Create Test Telemetry Config File       ${EXE_CONFIG_FILE}    ${CERT_PATH}    ${USERNAME}     POST
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}


Telemetry Executable Creates HTTP PUT Request
    [Documentation]    Telemetry Executable Creates HTTP PUT Request With Collected Telemetry Content
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}

Telemetry Executable Generates System Telemetry Without Cpu Cores
    [Documentation]    Telemetry Executable Generates System Telemetry when /usr/bin/lscpu fails to execute
    [Tags]  FAULTINJECTION  TELEMETRY
    Break System Command    /usr/bin/lscpu
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Restore System Commands
    Check System Telemetry Json Is Correct  ${telemetryFileContents}  cpu-cores


Telemetry Executable Telemetry Config File Certificate Path Empty
    [Documentation]    Telemetry Executable Creates Fails to Send Telemetry Data if Certificate Path Key in Config File is Empty
    Create Test Telemetry Config File     ${EXE_CONFIG_FILE}    ""  ${USERNAME}

    ${result} =    Run Process  sudo  -u  ${USERNAME}    ${SOPHOS_INSTALL}/base/bin/telemetry      ${EXE_CONFIG_FILE}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers   ${result.rc}       ${0}
    Check Telemetry Log Contains   Failed to contact telemetry server (5): SSL peer certificate or SSH remote key was not OK


Telemetry Executable Telemetry Config File Path Invalid
    [Documentation]    Telemetry Executable handles Invalid configuration file path and returns Failure return code
    ${result} =    Run Process  sudo  -u  ${USERNAME}    ${SOPHOS_INSTALL}/base/bin/telemetry      ${SOPHOS_INSTALL}/tmp/telemetry/nonexistenttelemetryconfigfile.jason

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers   ${result.rc}       ${FAILED}      Telemetry executable returned an unexpected error code


Telemetry Executable Telemetry Config File Data Size Exceeds Maximum Size
    [Documentation]    Telemetry Executable handles data larger than maximum size and returns failure return code
    Set Telemetry Config JsonMax    10
    Create Test Telemetry Config File     ${EXE_CONFIG_FILE}    ${CERT_PATH}    ${USERNAME}
    ${result} =    Run Process  sudo  -u  ${USERNAME}    ${SOPHOS_INSTALL}/base/bin/telemetry      ${EXE_CONFIG_FILE}

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers   ${result.rc}       ${FAILED}      Telemetry executable returned an unexpected error code


Telemetry Executable Generates System Telemetry When Command Hangs
    [Documentation]    Telemetry Executable Generates System Telemetry when /bin/df hangs
    [Tags]  FAULTINJECTION   TELEMETRY
    Hang System Command    /bin/df
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Restore System Commands
    ${result} =  Run Process  pkill -f 'sleep 99999'  shell=True
    Log  ${result.stdout}
    Check System Telemetry Json Is Correct  ${telemetryFileContents}  disks
    Check Telemetry Log Contains   Failed to get telemetry item: disks, from command: /bin/df, exception: Process execution timed out running:

Telemetry Executable Generates System Telemetry When Command crashes
    [Tags]  FAULTINJECTION   TELEMETRY
    Crash System Command    /bin/df
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Restore System Commands
    Check System Telemetry Json Is Correct  ${telemetryFileContents}  disks
    Check Telemetry Log Contains    Failed to get telemetry item: disks, from command: /bin/df, exception: Process execution returned non-zero exit code,

Telemetry Executable Generates System Telemetry When Command has wrong permissions
    [Tags]  FAULTINJECTION   TELEMETRY
    Permission Denied System Command    /bin/df
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Restore System Commands
    Check System Telemetry Json Is Correct  ${telemetryFileContents}  disks
    Check Telemetry Log Contains     Failed to get telemetry item: disks, from command: /bin/df, exception: Process execution returned non-zero exit code, 'Exit Code: [13] Permission denied'

Telemetry Executable Generates System Telemetry When Command writes too much to stdout
    [Tags]  FAULTINJECTION   TELEMETRY
    Overload System Command    /usr/sbin/getenforce
    #executable will segfault if input is not trimmed properly as std::regex can only handle about 28k
    # therefore test passes as long as the executable runs without crashing
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}

Telemetry Executable Invalid Json In Telemetry Config File
    [Documentation]    Telemetry Executable Creates Fails to Send Telemetry Data if Certificate Path Key in Config File is Empty
    Create Broken Telemetry Config File     ${EXE_CONFIG_FILE}   ${USERNAME}
    ${result} =    Run Process    ${SOPHOS_INSTALL}/base/bin/telemetry      ${EXE_CONFIG_FILE}

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers   ${result.rc}       ${FAILED}      Telemetry executable returned an unexpected error code


Telemetry Executable Uses Correct Configuration For Build Type
    # We will always use Fake Warehouse for this test so even with a prod build it should be linux/dev
    Check Log Contains    linux/dev    ${EXE_CONFIG_FILE}    telemetry-config.json

# The way version and endpoint ID are extracted is the same, they both use the ini file reader, therefore the tests
# below don't duplicate testing for version and endpoint id as they will behave in the same way.
# Customer ID is covered in unit tests as it is XML.

Telemetry Executable Handles Errors When Reading Missing MCS Config For Endpoint ID
    Drop ALC Policy Into Place
    Remove File  ${MCS_CONFIG}
    Should Not Exist  ${TELEMETRY_OUTPUT_JSON}
    Run Telemetry And Expect Field Missing  endpointId

Telemetry Executable Handles Errors When Reading MCS Config With Missing Key For Endpoint ID
    Drop ALC Policy Into Place
    Drop sophos-spl-local File Into Place  ${SUPPORT_FILES}/base_data/mcs.config-missing-mcs-id  ${MCS_CONFIG}
    Run Telemetry And Expect Field Missing  endpointId

Telemetry Executable Handles Errors When Reading Corrupt MCS Config For Endpoint ID
    Drop ALC Policy Into Place
    Drop sophos-spl-local File Into Place  ${SUPPORT_FILES}/base_data/garbage-text  ${MCS_CONFIG}
    Run Telemetry And Expect Field Missing  endpointId

Telemetry Executable Handles Errors When Reading Machine ID With Invalid Permissions
    Drop ALC Policy Into Place
    Run Process  chmod  600  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Run Process  chown  root:root  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Run Telemetry And Expect Field Missing   machineId
    Reset MachineID Permissions
    #move the last seen line forward to ignore errors caused by changing achine_id.txt permissions
    Mark Log File  ${SOPHOS_INSTALL}/logs/base/watchdog.log

Telemetry Executable Generates Watchdog Telemetry That Increments When Plugins Die And Are Restarted
    [Documentation]    Telemetry Executable Generates Watchdog Telemetry

    Wait Until Keyword Succeeds
    ...  30s
    ...  3s
    ...  Check Expected Base Processes Are Running

    Kill Sophos Processes That Arent Watchdog

    Wait Until Keyword Succeeds
    ...  40s
    ...  5s
    ...  Check Expected Base Processes Are Running

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}
    Check Watchdog Telemetry Json Is Correct  ${telemetryFileContents}  1  expected_code=${9}


Telemetry Executable Will Do A Successful HTTP PUT Request When Server Run TLSv1_2
    [Documentation]    Telemetry Executable Creates HTTP PUT Request With Collected Telemetry Content
    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable With HTTPS Protocol   TLSProtocol=tlsv1_2
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}

Telemetry Executable Will Do A Successful Send To Splunk With Defualt System Cert Store CA Certs
    [Documentation]    Telemetry Executable Will Do A Successful Send To Splunk
    Create Test Telemetry Config File  ${EXE_CONFIG_FILE}  server=t1.sophosupd.com
    Run Telemetry Executable    ${EXE_CONFIG_FILE}  ${SUCCESS}  checkResult=0


Telemetry Executable HTTP PUT Request Will Fail When Server Highest TLS is Less Than TLSv1_2
    [Documentation]    Telemetry Executable Creates HTTP PUT Request With Collected Telemetry Content
    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable With HTTPS Protocol   TLSProtocol=tlsv1_1

    ${result} =    Run Process  sudo  -u  ${USERNAME}    ${SOPHOS_INSTALL}/base/bin/telemetry      ${EXE_CONFIG_FILE}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers   ${result.rc}       ${0}
    Check Telemetry Log Contains   Failed to contact telemetry server (5): SSL connect error

Test With Proxy
    [Teardown]  Teardown With Proxy Clear
    Start Proxy Server With Basic Auth    3000    username   password
    Create file   /opt/sophos-spl/base/etc/sophosspl/current_proxy  {"proxy": "localhost:3000", "credentials": "CCDHOz/vaDoMYy4SMijJh2K3ur0RB+w1Z+zNeJOq2dGM9X2+ZNqHKz1qri2KFGltImEpsGXGFJGkyhSeAbkYXcM6"}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check System Telemetry Json Is Correct  ${telemetryFileContents}  None  Proxy


Test Outbreak Mode Telemetry
    [Teardown]  Stop Fake Plugin Teardown
    Management Agent Test Setup

    Set Log Level For Component And Reset and Return Previous Log  sophos_managementagent  DEBUG
    Set Log Level For Component And Reset and Return Previous Log  telemetry  DEBUG

    Remove Event Xml Files

    Setup Plugin Registry
    ${mark} =  mark_log_size   ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    Start Management Agent

    Start Plugin
    set_fake_plugin_app_id  CORE

    wait for log contains from mark   ${mark}     Starting service health checks     ${120}

    # Pre-outbreak mode
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Should contain  ${telemetryFileContents}  "outbreak-ever":"false"
    Should not contain  ${telemetryFileContents}  "outbreak-now"
    Should not contain  ${telemetryFileContents}  "outbreak-today"

    # Enter outbreak mode
    ${eventContent} =  Get File  ${SUPPORT_FILES}/CORE_events/detection.xml
    Enter Outbreak Mode  ${eventContent}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Should contain  ${telemetryFileContents}  "outbreak-ever":"true"
    Should contain  ${telemetryFileContents}  "outbreak-now":"true"
    Should contain  ${telemetryFileContents}  "outbreak-today":"true"

    # Exit outbreak mode
    ${mark} =  mark_log_size  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log
    ${uuid} =  Set Variable  5df69683-a5a2-5d96-897d-06f9c4c8c7bf
    Send Clear Action  ${uuid}
    wait for log contains from mark  ${mark}  Leaving outbreak mode

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Should contain  ${telemetryFileContents}  "outbreak-ever":"true"
    Should contain  ${telemetryFileContents}  "outbreak-now":"false"
    Should contain  ${telemetryFileContents}  "outbreak-today":"true"


Telemetry Executable Moves ESM to Top Level When ESM Not Enabled
    [Tags]  SMOKE  TELEMETRY  TAP_PARALLEL2
    [Documentation]    Telemetry Executable Generates Telemetry

    Cleanup Telemetry Server
    Require Fresh Install

    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG}  Starting service health checks  timeout=${120}

    Create Empty SulDownloader Config
    setup mcs config with JWT token
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Drop ALC Policy Into Place
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Processing Flags    ${10}

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    LOG    ${telemetryFileContents}

    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  0
        ...    set_edr=True
        ...    set_av=True
        ...    install_state=0
        ...    download_state=0
    check_base_telemetry_json_is_correct  ${telemetryFileContents}

    check_for_key_value_in_top_level_telemetry    ${telemetryFileContents}    esmName   ${EMPTY}
    check_for_key_value_in_top_level_telemetry    ${telemetryFileContents}    esmToken   ${EMPTY}


Telemetry Executable Moves ESM to Top Level When ESM Enabled
    [Tags]  SMOKE  TELEMETRY  TAP_PARALLEL2

    ${esmname} =  Set Variable   LTS 2023.1.1
    ${esmtoken} =    Set Variable    f4d41a16-b751-4195-a7b2-1f109d49469d

    Cleanup Telemetry Server
    Require Fresh Install

    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG}  Starting service health checks  timeout=${120}

    Create Empty SulDownloader Config
    setup mcs config with JWT token
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Drop ALC Policy With ESM Into Place     ${esmname}    ${esmtoken}
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Processing Flags    ${10}

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    LOG    ${telemetryFileContents}

    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  0
    ...    base_fixed_version=${esmname}
    ...    install_state=0
    ...    download_state=0
    check_base_telemetry_json_is_correct  ${telemetryFileContents}

    check_for_key_value_in_top_level_telemetry    ${telemetryFileContents}    esmName   ${esmname}
    check_for_key_value_in_top_level_telemetry    ${telemetryFileContents}    esmToken   ${esmtoken}