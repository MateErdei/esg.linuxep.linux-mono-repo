*** Settings ***
Documentation     Test the Telemetry executable

Library           ${LIBS_DIRECTORY}/CentralUtils.py
Library           ${LIBS_DIRECTORY}/MCSRouter.py
Library           ${LIBS_DIRECTORY}/SystemInfo.py
Library           OperatingSystem
Library           ${LIBS_DIRECTORY}/LogUtils.py

Resource  TelemetryResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../installer/InstallerResources.robot
Resource  ../mcs_router/McsRouterResources.robot


Suite Setup      Setup Telemetry Tests
Suite Teardown   Cleanup Telemetry Tests

Test Setup       Telemetry Test Setup
Test Teardown    Telemetry Test Teardown

Default Tags  TELEMETRY


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
    Create File    ${SOPHOS_INSTALL}/base/update/var/update_config.json

Telemetry Test Setup
    Prepare To Run Telemetry Executable
    Drop MCS Config Into Place
    Stop Watchdog
    Remove File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Start Watchdog

Telemetry Test Teardown
    Reset MachineID Permissions
    General Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Restore System Commands
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Reset MachineID Permissions
    Remove File   ${SOPHOS_INSTALL}/base/update/var/update_config.json
    Remove File   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Teardown With Proxy Clear
    Remove File   /opt/sophos-spl/base/etc/sophosspl/current_proxy
    Telemetry Test Teardown



Reset MachineID Permissions
    Run Process  chmod  640  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/machine_id.txt



*** Test Cases ***
Telemetry Executable Generates System Telemetry
    [Tags]  SMOKE  TELEMETRY  TAP_TESTS
    [Documentation]    Telemetry Executable Generates System Telemetry
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}
    Check System Telemetry Json Is Correct  ${telemetryFileContents}

Telemetry Executable Generates Watchdog Telemetry
    [Tags]  SMOKE  TAP_TESTS   TELEMETRY   WATCHDOG
    [Documentation]    Telemetry Executable Generates Watchdog Telemetry
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    log  ${telemetryFileContents}
    Check Watchdog Telemetry Json Is Correct  ${telemetryFileContents}

Telemetry Executable Generates Base Telemetry
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check Base Telemetry Json Is Correct  ${telemetryFileContents}

Telemetry Executable Generates Update Scheduler Telemetry
    # Make sure there are no left over update telemetry items.
    Cleanup Telemetry Server
    Require Fresh Install

    Create Empty SulDownloader Config

    Drop ALC Policy Into Place

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  0   sddsid=NotAUserName

Telemetry Executable Generates Update Scheduler Telemetry With Fixed Version Policy
    # Make sure there are no left over update telemetry items.
    Cleanup Telemetry Server
    Require Fresh Install

    Create Empty SulDownloader Config

    Drop ALC Policy With Fixed Version Into Place

    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}   0   fixed_version=1.0.2  sddsid=NotAUserName

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
    Should Be Equal As Integers   ${result.rc}       ${FAILED}      Telemetry executable returned an unexpected error code


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
    Remove File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Should Not Exist  ${TELEMETRY_OUTPUT_JSON}
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_OUTPUT_JSON}
    Check Base Telemetry Json Is Correct  ${telemetryFileContents}  endpointId

Telemetry Executable Handles Errors When Reading MCS Config With Missing Key For Endpoint ID
    Drop ALC Policy Into Place
    Drop sophos-spl-user File Into Place  ${SUPPORT_FILES}/base_data/mcs.config-missing-mcs-id  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_OUTPUT_JSON}
    Check Base Telemetry Json Is Correct  ${telemetryFileContents}  endpointId

Telemetry Executable Handles Errors When Reading Corrupt MCS Config For Endpoint ID
    Drop ALC Policy Into Place
    Drop sophos-spl-user File Into Place  ${SUPPORT_FILES}/base_data/garbage-text  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_OUTPUT_JSON}
    Check Base Telemetry Json Is Correct  ${telemetryFileContents}  endpointId

Telemetry Executable Handles Errors When Reading Machine ID With Invalid Permissions
    Drop ALC Policy Into Place
    Run Process  chmod  600  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Run Process  chown  root:root  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_OUTPUT_JSON}
    Check Base Telemetry Json Is Correct  ${telemetryFileContents}  machineId
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
    Check Watchdog Telemetry Json Is Correct  ${telemetryFileContents}  1


Telemetry Executable Will Do A Successful HTTP PUT Request When Server Run TLSv1_2
    [Documentation]    Telemetry Executable Creates HTTP PUT Request With Collected Telemetry Content
    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable With HTTPS Protocol   TLSProtocol=tlsv1_2
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${SUCCESS}


Telemetry Executable HTTP PUT Request Will Fail When Server Highest TLS is Less Than TLSv1_2
    [Documentation]    Telemetry Executable Creates HTTP PUT Request With Collected Telemetry Content
    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable With HTTPS Protocol   TLSProtocol=tlsv1_1
    Run Telemetry Executable    ${EXE_CONFIG_FILE}     ${FAILED}   checkResult=0

    Wait Until Keyword Succeeds
    ...     5 seconds
    ...     1 seconds
    ...     Check Log Contains   Response HttpCode: 35   ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log   TelemetryLog

    Wait Until Keyword Succeeds
    ...     5 seconds
    ...     1 seconds
    ...     Check Log Contains   SSL connect error   ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log   TelemetryLog

Test With Proxy
    [Teardown]  Teardown With Proxy Clear
    Start Proxy Server With Basic Auth    3000    username   password
    Create file   /opt/sophos-spl/base/etc/sophosspl/current_proxy  {"proxy": "localhost:3000", "credentials": "CCDHOz/vaDoMYy4SMijJh2K3ur0RB+w1Z+zNeJOq2dGM9X2+ZNqHKz1qri2KFGltImEpsGXGFJGkyhSeAbkYXcM6"}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check System Telemetry Json Is Correct  ${telemetryFileContents}

    Check Log Contains   Setup proxy for the connection    ${SOPHOS_INSTALL}/var/sophos-spl-comms/logs/base/comms-network.log    comms network