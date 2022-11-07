*** Settings ***
Force Tags      INTEGRATION  AV_HEALTH
Library         Collections
Library         ../Libs/SystemFileWatcher.py

Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/OnAccessResources.robot

Suite Setup     AV Health Suite Setup
Suite Teardown  Uninstall All

Test Setup      AV Health Test Setup
Test Teardown   AV Health Test Teardown

*** Variables ***
&{SERVICE_HEALTH_STATUSES}
...   0=${0}     1=${1}     2=${2}
...   ${0}=${0}  ${1}=${1}  ${2}=${2}
...   GOOD=${0}  BAD=${1}   MISSING=${2}
...   Good=${0}  Bad=${1}   Missing=${2}
...   good=${0}  bad=${1}   missing=${2}

&{THREAT_HEALTH_STATUSES}
...   1=${1}     2=${2}            3=${3}
...   ${1}=${1}  ${2}=${2}         ${3}=${3}
...   GOOD=${1}  SUSPICIOUS=${2}   BAD=${3}
...   Good=${1}  Suspicious=${2}   Bad=${3}
...   good=${1}  suspicious=${2}   bad=${3}

*** Keywords ***
AV Health Suite Setup
    Install With Base SDDS

AV Health Test Setup
    SystemFileWatcher.Start Watching System Files
    Register Cleanup      SystemFileWatcher.stop watching system files

    AV And Base Setup
    Wait Until AV Plugin running
    Wait Until threat detector running

    Create File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector
    Register Cleanup  Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

AV Health Test Teardown
    Remove File    /tmp_test/naughty_eicar
    AV And Base Teardown
    Uninstall All

SHS Status File Contains
    [Arguments]  ${content_to_contain}
    ${shsStatus} =  Get File   ${MCS_DIR}/status/SHS_status.xml
    Log  ${shsStatus}
    Should Contain  ${shsStatus}  ${content_to_contain}
    
Check Status Health is Reporting Correctly
    [Arguments]    ${healthStatus}

    ${healthStatus} =   Get From Dictionary  ${SERVICE_HEALTH_STATUSES}  ${healthStatus}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check AV Telemetry    health    ${healthStatus}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  SHS Status File Contains   <detail name="Sophos Linux AntiVirus" value="${healthStatus}" />

Check Threat Health is Reporting Correctly
    [Arguments]    ${threatStatus}

    ${threatStatus} =   Get From Dictionary  ${THREAT_HEALTH_STATUSES}  ${threatStatus}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check AV Telemetry    threatHealth    ${threatStatus}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  SHS Status File Contains   <item name="threat" value="${threatStatus}" />

*** Test Cases ***
AV Not Running Triggers Bad Status Health
    Check Status Health is Reporting Correctly    GOOD

    Stop AV Plugin
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  SHS Status File Contains   <detail name="Sophos Linux AntiVirus" value="2" />

    Start AV Plugin
    Wait until AV Plugin running
    Check Status Health is Reporting Correctly    GOOD

Sophos Threat Detector Not Running Triggers Bad Status Health
    Check Status Health is Reporting Correctly    GOOD

    Stop sophos_threat_detector
    Check Status Health is Reporting Correctly    BAD

    Start sophos_threat_detector
    Wait until threat detector running
    Check Status Health is Reporting Correctly    GOOD

Sophos Threat Detector Crashing Triggers Bad Health
    # test that a stale pidfile doesn't lead to incorrect health status
    Check Status Health is Reporting Correctly    GOOD

    Ignore Coredumps and Segfaults
    Register Cleanup    Exclude Threat Detector Launcher Died With 11

    ${pid} =   Record Sophos Threat Detector PID
    Run Process   /bin/kill   -SIGSEGV   ${pid}
    Stop sophos_threat_detector

    ${PID_FILE} =  Set Variable  ${AV_PLUGIN_PATH}/chroot/var/threat_detector.pid
    File Should Exist   ${PID_FILE}

    Check Status Health is Reporting Correctly    BAD

    Start sophos_threat_detector
    Wait until threat detector running
    Check Status Health is Reporting Correctly    GOOD


Sophos On-Access Process Not Running Triggers Bad Status Health
    Check Status Health is Reporting Correctly    GOOD

    Stop soapd
    Check Status Health is Reporting Correctly    BAD

    Start soapd
    Wait Until On Access running
    Check Status Health is Reporting Correctly    GOOD


Sophos On-Access Process Crashing Triggers Bad Health
    Check Status Health is Reporting Correctly    GOOD

    Ignore Coredumps and Segfaults
    Register Cleanup    Exclude Soapd Died With 11

    ${pid} =   Record Soapd Plugin PID
    Run Process   /bin/kill   -SIGSEGV   ${pid}
    Stop soapd

    ${PID_FILE} =  Set Variable  ${AV_PLUGIN_PATH}/var/soapd.pid
    File Should Exist   ${PID_FILE}

    Check Status Health is Reporting Correctly    BAD

    Start soapd
    Wait Until On Access running
    Check Status Health is Reporting Correctly    GOOD


Sophos SafeStore Process Not Running Does Not Trigger Bad Status Health When SafeStore Is Not Enabled
    Mark AV Log
    Send Flags Policy To Base  flags_policy/flags.json
    Check Status Health is Reporting Correctly    GOOD

    Stop SafeStore
    AV Plugin Log Contains With Offset  SafeStore flag not set. Setting SafeStore to disabled.
    Check Status Health is Reporting Correctly    GOOD

    Start SafeStore
    Wait Until SafeStore running
    Check Status Health is Reporting Correctly    GOOD

Sophos SafeStore Process Not Running Triggers Bad Status Health
    Mark AV Log
    Send Flags Policy To Base  flags_policy/flags_enabled.json
    Check Status Health is Reporting Correctly    GOOD

    Stop SafeStore
    AV Plugin Log Contains With Offset  SafeStore flag set. Setting SafeStore to enabled.
    Check Status Health is Reporting Correctly    BAD

    Start SafeStore
    Wait Until SafeStore running
    Check Status Health is Reporting Correctly    GOOD

Sophos SafeStore Crashing Triggers Bad Health
    Mark AV Log
    Send Flags Policy To Base  flags_policy/flags_enabled.json
    Wait Until AV Plugin Log Contains With Offset
    ...   SafeStore flag set. Setting SafeStore to enabled.
    ...   timeout=60

    Check Status Health is Reporting Correctly    GOOD

    Ignore Coredumps and Segfaults
    Register Cleanup    Exclude SafeStore Died With 11

    ${pid} =   Record SafeStore Plugin PID
    Run Process   /bin/kill   -SIGSEGV   ${pid}
    Stop SafeStore

    ${PID_FILE} =  Set Variable  ${AV_PLUGIN_PATH}/var/safestore.pid
    File Should Exist   ${PID_FILE}

    Check Status Health is Reporting Correctly    BAD

    Start SafeStore
    Wait Until SafeStore running
    Check Status Health is Reporting Correctly    GOOD


Clean CLS Result Does Not Reset Threat Health
    Check Threat Health is Reporting Correctly    GOOD

    Create File     /tmp_test/naughty_eicar    ${EICAR_STRING}
    Create File     /tmp_test/clean_file       ${CLEAN_STRING}

    mark av log
    mark sophos threat detector log

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/naughty_eicar
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset   Detected "EICAR-AV-Test" in /tmp_test/naughty_eicar
    Wait Until AV Plugin Log Contains With Offset  Threat health changed to suspicious

    Check Threat Health is Reporting Correctly    SUSPICIOUS

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/clean_file
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Check Threat Health is Reporting Correctly    SUSPICIOUS

Bad Threat Health is preserved after av plugin restarts
    Check Threat Health is Reporting Correctly    GOOD

    Create File     /tmp_test/naughty_eicar    ${EICAR_STRING}

    Configure and check scan now with offset
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

    Check Threat Health is Reporting Correctly    SUSPICIOUS
    Stop AV Plugin
    Start AV Plugin
    Wait until AV Plugin running
    Check Threat Health is Reporting Correctly    SUSPICIOUS

Clean Scan Now Result Does Not Reset Threat Health
    Check Threat Health is Reporting Correctly    GOOD

    Create File     /tmp_test/naughty_eicar    ${EICAR_STRING}

    Mark AV Log
    Configure and check scan now with offset
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    AV Plugin Log Contains With Offset   Completed scan Scan Now and detected threats

    Check Threat Health is Reporting Correctly    SUSPICIOUS

    Remove File  /tmp_test/naughty_eicar

    Mark AV Log
    Configure and check scan now with offset
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180
    AV Plugin Log Contains With Offset   Completed scan Scan Now without detecting any threats

    Check Threat Health is Reporting Correctly    SUSPICIOUS


Clean Scheduled Scan Result Does Not Resets Threat Health
    Check Threat Health is Reporting Correctly    GOOD

    Create File  /tmp_test/naughty_eicar  ${EICAR_STRING}

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=210

    Check Threat Health is Reporting Correctly    SUSPICIOUS

    Remove File  /tmp_test/naughty_eicar

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=210

    Check Threat Health is Reporting Correctly    SUSPICIOUS

Test av health is green right after install
    Install With Base SDDS
    Wait Until Management Log Contains   Starting service health checks   timeout=45

    Wait until AV Plugin running
    Wait until threat detector running
    AV Plugin Log Does Not Contain       Health found previous Sophos Threat Detector process no longer running:

    SHS Status File Contains    <detail name="Sophos Linux AntiVirus" value="0" />


AV health is unaffected by scanning the threat_detector pidfile
    Check Status Health is Reporting Correctly    GOOD

    ${PID_FILE} =  Set Variable  ${AV_PLUGIN_PATH}/chroot/var/threat_detector.pid
    File Should Exist   ${PID_FILE}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${PID_FILE}
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Check Status Health is Reporting Correctly    GOOD

AV health is unaffected by on-access scanning the soapd pidfile
    Register Cleanup  Exclude On Access Scan Errors

    ${mark} =  get_on_access_log_mark
    Mark On Access Log
    Send Policies to enable on-access
    Wait for on access to be enabled  mark=${mark}

    # this will cause on-access events for the pid file
    Check Status Health is Reporting Correctly    GOOD

    ${PID_FILE} =  Set Variable  ${AV_PLUGIN_PATH}/var/soapd.pid
    File Should Exist   ${PID_FILE}

    Mark On Access Log
    Get File   ${PID_FILE}
    Wait Until On Access Log Contains With Offset  On-open event for ${PID_FILE} from

    Check Status Health is Reporting Correctly    GOOD
