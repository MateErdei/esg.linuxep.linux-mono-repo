*** Settings ***
Force Tags      INTEGRATION  AV_HEALTH
Library         Collections
Library         ../Libs/LogUtils.py
Library         ../Libs/SystemFileWatcher.py

Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/OnAccessResources.robot
Resource        ../shared/SafeStoreResources.robot

Suite Setup     AV Health Suite Setup
Suite Teardown  Uninstall All

Test Setup      AV Health Test Setup
Test Teardown   AV Health Test Teardown

Default Tags    TAP_TESTS

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

${MANAGEMENT_AGENT_HEALTH_STARTUP_DELAY} =  120
${SHS_STATUS_FILE} =  ${MCS_DIR}/status/SHS_status.xml

*** Keywords ***
AV Health Suite Setup
    Install With Base SDDS

AV Health Test Setup
    SystemFileWatcher.Start Watching System Files
    Register Cleanup      SystemFileWatcher.stop watching system files

    AV And Base Setup
    Wait Until AV Plugin running
    Wait Until threat detector running
    Send Policies to disable on-access
    File Should Not Exist  ${AV_PLUGIN_PATH}/var/onaccess_unhealthy_flag

    Create File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector
    Register Cleanup  Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

AV Health Test Teardown
    Remove File    /tmp_test/naughty_eicar
    AV And Base Teardown

Wait until SHS Status File created
    Wait until created  ${SHS_STATUS_FILE}  timeout=${MANAGEMENT_AGENT_HEALTH_STARTUP_DELAY} secs

SHS Status File Contains
    [Arguments]  ${content_to_contain}
    ${shsStatus} =  Get File   ${SHS_STATUS_FILE}
    Log  ${shsStatus}
    Should Contain  ${shsStatus}  ${content_to_contain}
    
Check Status Health is Reporting Correctly
    [Arguments]    ${healthStatus}

    ${healthStatus} =   Get From Dictionary  ${SERVICE_HEALTH_STATUSES}  ${healthStatus}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check AV Telemetry    health    ${healthStatus}

    Wait until SHS Status File created

    Wait Until Keyword Succeeds
    ...  ${MANAGEMENT_AGENT_HEALTH_STARTUP_DELAY} secs
    ...  5 secs
    ...  SHS Status File Contains   <detail name="Sophos Linux AntiVirus" value="${healthStatus}" />

Check Threat Health is Reporting Correctly
    [Arguments]    ${threatStatus}

    ${threatStatus} =   Get From Dictionary  ${THREAT_HEALTH_STATUSES}  ${threatStatus}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check AV Telemetry    threatHealth    ${threatStatus}

    Wait until SHS Status File created

    Wait Until Keyword Succeeds
    ...  ${MANAGEMENT_AGENT_HEALTH_STARTUP_DELAY} secs
    ...  5 secs
    ...  SHS Status File Contains   <item name="threat" value="${threatStatus}" />

*** Test Cases ***

# Must be first test case to get clean install result
Test av health is green right after install
    wait_for_log_contains_after_last_restart  ${MANAGEMENT_AGENT_LOG_PATH}  Starting service health checks   timeout=120

    Wait until AV Plugin running
    Wait until threat detector running
    AV Plugin Log Does Not Contain       Health found previous Sophos Threat Detector process no longer running:

    SHS Status File Contains    <detail name="Sophos Linux AntiVirus" value="0" />



AV Not Running Triggers Bad Status Health
    Check Status Health is Reporting Correctly    GOOD

    Stop AV Plugin
    Wait until SHS Status File created

    Wait Until Keyword Succeeds
    ...  ${MANAGEMENT_AGENT_HEALTH_STARTUP_DELAY} secs
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
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags.json
    Check Status Health is Reporting Correctly    GOOD

    Stop SafeStore
    Check AV Log Contains After Mark  SafeStore flag not set. Setting SafeStore to disabled.  ${av_mark}
    Check Status Health is Reporting Correctly    GOOD

    Start SafeStore
    Wait Until SafeStore running
    Check Status Health is Reporting Correctly    GOOD

Sophos SafeStore Process Not Running Triggers Bad Status Health
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Check Status Health is Reporting Correctly    GOOD

    Stop SafeStore
    Check AV Log Contains After Mark  SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}
    Check Status Health is Reporting Correctly    BAD

    Start SafeStore
    Wait Until SafeStore running
    Check Status Health is Reporting Correctly    GOOD

Sophos SafeStore Crashing Triggers Bad Health
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For AV Log Contains After Mark
    ...   SafeStore flag set. Setting SafeStore to enabled.
    ...   ${av_mark}
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

    ${av_mark} =  Get AV Log Mark
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/naughty_eicar
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Wait For Sophos Threat Detector Log Contains After Mark   Detected "EICAR-AV-Test" in /tmp_test/naughty_eicar  ${threat_detector_mark}
    Wait For AV Log Contains After Mark  Threat health changed to suspicious  ${av_mark}

    Check Threat Health is Reporting Correctly    SUSPICIOUS

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/clean_file
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Check Threat Health is Reporting Correctly    SUSPICIOUS

Bad Threat Health is preserved after av plugin restarts
    Check Threat Health is Reporting Correctly    GOOD

    Create File     /tmp_test/naughty_eicar    ${EICAR_STRING}

    ${av_mark} =  Get AV Log Mark
    Configure and run scan now
    wait_for_log_contains_from_mark  ${av_mark}  Completed scan  timeout=180

    Check Threat Health is Reporting Correctly    SUSPICIOUS
    Stop AV Plugin
    Start AV Plugin
    Wait until AV Plugin running
    Check Threat Health is Reporting Correctly    SUSPICIOUS

Clean Scan Now Result Does Not Reset Threat Health
    Check Threat Health is Reporting Correctly    GOOD

    Create File     /tmp_test/naughty_eicar    ${EICAR_STRING}

    ${av_mark} =  Get AV Log Mark
    Configure and run scan now
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180
    Check AV Log Contains After Mark   Completed scan Scan Now and detected threats  ${av_mark}

    Check Threat Health is Reporting Correctly    SUSPICIOUS

    Remove File  /tmp_test/naughty_eicar

    ${av_mark} =  Get AV Log Mark
    Configure and run scan now
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180
    Check AV Log Contains After Mark   Completed scan Scan Now without detecting any threats  ${av_mark}

    Check Threat Health is Reporting Correctly    SUSPICIOUS


Clean Scheduled Scan Result Does Not Reset Threat Health
    Check Threat Health is Reporting Correctly    GOOD

    Create File  /tmp_test/naughty_eicar  ${EICAR_STRING}

    ${av_mark} =  Get AV Log Mark
    Send CORC Policy to Disable SXL
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until Scheduled Scan Updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark}  timeout=180
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=210

    Check Threat Health is Reporting Correctly    SUSPICIOUS

    Remove File  /tmp_test/naughty_eicar

    ${av_mark2} =  Get AV Log Mark
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait Until Scheduled Scan Updated After Mark  ${av_mark2}
    Wait For AV Log Contains After Mark  Starting scan Sophos Cloud Scheduled Scan  ${av_mark2}  timeout=180
    Wait For AV Log Contains After Mark  Completed scan  ${av_mark2}  timeout=210

    Check Threat Health is Reporting Correctly    SUSPICIOUS

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

    Send Policies to enable on-access

    # this will cause on-access events for the pid file
    Check Status Health is Reporting Correctly    GOOD

    ${PID_FILE} =  Set Variable  ${AV_PLUGIN_PATH}/var/soapd.pid
    File Should Exist   ${PID_FILE}

    ${oa_mark} =  Get on access log mark
    Get File   ${PID_FILE}
    wait_for_log_contains_from_mark  ${oa_mark}  On-open event for ${PID_FILE} from

    Check Status Health is Reporting Correctly    GOOD


AV Service Health Turns Red When SUSI Fails Initialisation And Turns Green When SUSI Recovers
    [Tags]  susi_init
    Register Cleanup  Exclude Susi Initialisation Failed Messages On Access Enabled

    Send Policies to enable on-access

    #necessary for error markers
    Create File   ${OA_LOCAL_SETTINGS}   { "numThreads" : 1 }
    Register Cleanup    Remove File    ${OA_LOCAL_SETTINGS}
    Restart soapd

    ${td_mark} =    Get Sophos Threat Detector Log Mark
    Create SUSI Initialisation Error

    On-access Scan Clean File

    Wait For Sophos Threat Detector Log Contains After Mark   ScanningServerConnectionThread aborting scan, failed to initialise SUSI  ${td_mark}
    File Should Exist    ${AV_PLUGIN_PATH}/chroot/var/threatdetector_unhealthy_flag
    Check Status Health is Reporting Correctly    BAD

    ${td_mark} =  Get Sophos Threat Detector Log Mark

    Move Directory  ${VDL_TEMP_DESTINATION}  ${VDL_DIRECTORY}

    On-access Scan Clean File

    Wait For Sophos Threat Detector Log Contains After Mark   ScanningServerConnectionThread has created a new scanner   ${td_mark}
    File Should Not Exist    ${AV_PLUGIN_PATH}/chroot/var/threatdetector_unhealthy_flag
    Check Status Health is Reporting Correctly    GOOD