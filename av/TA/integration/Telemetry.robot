*** Settings ***
Documentation    Telemetry tests
Force Tags      INTEGRATION  AVBASE  TELEMETRY  TAP_PARALLEL1
Library         Collections
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ${COMMON_TEST_LIBS}/OnFail.py
Library         ../Libs/ThreatReportUtils.py
Library         ../Libs/Telemetry.py

Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/OnAccessResources.robot

Suite Setup     Telemetry Suite Setup
Suite Teardown  Telemetry Suite Teardown

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Keywords ***
Telemetry Suite Setup
    Install With Base SDDS
    Send Alc Policy
    Send Sav Policy With No Scheduled Scans

Telemetry Suite Teardown
    Uninstall All

Log Telemetry files
    ${result} =  Run Process  ls -ld ${SSPL_BASE}/bin/telemetry ${SSPL_BASE}/bin ${SSPL_BASE}/bin/telemetry.*  shell=True  stdout=/tmp/telemetry.files  stderr=STDOUT
    Log  Telemetry files: ${result.stdout} code ${result.rc}
    Debug Telemetry  ${SSPL_BASE}/bin/telemetry

*** Test Cases ***

AV plugin runs scheduled scan and updates telemetry
    [Tags]  SLOW  RHEL7
    #Register On Fail  Log Telemetry files
    Log Telemetry files

    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol    port=${4421}

    ${av_mark} =  Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait For AV Log Contains After Mark  Completed scan  ${av_mark}  timeout=180

    Check AV Telemetry    scheduled-scan-count   ${1}


AV Plugin Can Send Telemetry
    # Assumes threat health is 1 (good)
    Run Telemetry Executable With HTTPS Protocol    port=${4431}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV Plugin sends non-zero processInfo to Telemetry
    # System file changes can cause ThreatDetector to be restarted while telemetry
    # is being fetched
    Create File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector
    Register Cleanup  Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

    Restart Sophos Threat Detector
    Run Telemetry Executable With HTTPS Protocol    port=${4432}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    ${memUsage}=    Get From Dictionary   ${avDict}   threatMemoryUsage
    ${processAge}=    Get From Dictionary   ${avDict}   threatProcessAge

    Should Not Be Equal As Integers  ${memUsage}  ${0}
    Should Be True  ${0} < ${processAge} < ${10}


AV Plugin Scan Now Updates Telemetry Count
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    # run a scan, count should increase to 1
    Configure and run scan now

    Check AV Telemetry    scan-now-count   ${1}

    av_log_contains_only_one_no_saved_telemetry_per_start


AV plugin Saves and Restores Scan Now Counter
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol    port=${4433}

    # run a scan, count should increase to 1
    Configure and run scan now

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   ${1}
    Dictionary Should Contain Item   ${rootkeyDict}   threatHealth   ${1}

    ${av_mark} =  Mark AV Log
    Start AV Plugin
    Wait For AV Log Contains After Mark  Restoring telemetry from disk for plugin: av  ${av_mark}

    Check AV Telemetry    scan-now-count   ${1}
    Check AV Telemetry    threatHealth   ${1}


AV plugin increments Scan Now Counter after Save and Restore
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    # run a scan, count should increase to 1
    Configure and run scan now

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   ${1}

    Start AV Plugin

    # run a scan, count should increase to 1
    Configure and run scan now

    Check AV Telemetry    scan-now-count   ${2}

On Access Provides Mount Info Telemetry
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${mark} =  Get on access log mark

    #This registers disable with cleanup
    Send Policies to enable on-access
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${oaDict} =    Set Variable     ${telemetryJson['on_access_process']}

    ${mounts} =     get_from_dictionary   ${oaDict}   file-system-types
    Should Contain    ${mounts}    tmpfs


On Access Provides Event Telemetry
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${mark} =  Get on access log mark

    #This registers disable with cleanup
    Send Policies to enable on-access
    wait for on access log contains after mark  On-access scanning enabled  mark=${mark}

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${oaDict} =    Set Variable     ${telemetryJson['on_access_process']}

    ${ratio} =    get_from_dictionary   ${oaDict}   ratio-of-dropped-events
    Should Be True   isinstance($ratio, float)
    Should Be True   0.0 <= ${ratio} <= 100.0


On Access Provides Scan Telemetry
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${mark} =  Get on access log mark

    #This registers disable with cleanup
    Send Policies to enable on-access
    wait for on access log contains after mark  On-access scanning enabled  mark=${mark}

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${oaDict} =    Set Variable     ${telemetryJson['on_access_process']}

    ${ratio} =    get_from_dictionary   ${oaDict}   ratio-of-scan-errors
    Should Be True   isinstance($ratio, float)
    Should Be True   0.0 <= ${ratio} <= 100.0


On Access ML Scanning Is Reported Correctly To Telemetry
    ${soapd_mark} =  Get on access log mark

    #This registers disable with cleanup and enabled ML scanning
    Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    wait for on access log contains after mark  On-access scanning enabled  mark=${soapd_mark}

    Check AV Telemetry    ml-scanning-enabled   True

    ${av_mark} =  Mark AV Log
    Send CORE Policy To Base  core_policy/CORE-36_ml_disabled.xml
    Wait for av log contains after mark     Machine Learning detections disabled  mark=${av_mark}

    Check AV Telemetry    ml-scanning-enabled   False


On Access Status Is Represented In Telemetry
    ${soapd_mark} =  Get on access log mark

    Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    wait for on access log contains after mark  On-access scanning enabled  mark=${soapd_mark}

    Check AV Telemetry    on-access-status    True

    ${soapd_mark} =  Get on access log mark
    Send CORE Policy To Base  core_policy/CORE-36_oa_disabled.xml
    wait for on access log contains after mark      On-access scanning disabled  mark=${soapd_mark}

    Check AV Telemetry   on-access-status   False