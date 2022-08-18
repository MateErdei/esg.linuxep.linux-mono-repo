*** Settings ***
Documentation    Telemetry tests
Force Tags      INTEGRATION  AVBASE  TELEMETRY
Library         Collections
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/ThreatReportUtils.py
Library         ../Libs/Telemetry.py

Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/AVAndBaseResources.robot

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

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

    Run Telemetry Executable With HTTPS Protocol    port=${4421}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   1


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

    Restart sophos_threat_detector and mark logs
    Run Telemetry Executable With HTTPS Protocol    port=${4432}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    ${memUsage}=    Get From Dictionary   ${avDict}   threatMemoryUsage
    ${processAge}=    Get From Dictionary   ${avDict}   threatProcessAge

    Should Not Be Equal As Integers  ${memUsage}  0
    Should Be True  ${processAge} > 0 and ${processAge} < 10

AV plugin Saves and Restores Scan Now Counter
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol    port=${4433}

    # run a scan, count should increase to 1
    Configure and check scan now with offset

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   1
    Dictionary Should Contain Item   ${rootkeyDict}   threatHealth   1

    Mark AV Log
    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  Restoring telemetry from disk for plugin: av
    Run Telemetry Executable With HTTPS Protocol    port=${4434}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   scan-now-count   1
    Dictionary Should Contain Item   ${avDict}   threatHealth   1


AV plugin increments Scan Now Counter after Save and Restore
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    # run a scan, count should increase to 1
    Configure and check scan now with offset

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   1

    Start AV Plugin

    # run a scan, count should increase to 1
    Configure and check scan now with offset

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   2


Telemetry Counters Are Zero by default
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol    port=${4436}
    Remove File   ${TELEMETRY_OUTPUT_JSON}

    Run Telemetry Executable With HTTPS Protocol    port=${4436}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   threat-eicar-count   0
    Dictionary Should Contain Item   ${avDict}   threat-count   0
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   0
    Dictionary Should Contain Item   ${avDict}   scan-now-count   0